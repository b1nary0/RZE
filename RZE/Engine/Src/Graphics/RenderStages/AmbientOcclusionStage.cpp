#include <Graphics/RenderStages/AmbientOcclusionStage.h>

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Vector4D.h>

namespace
{
	static_assert(sizeof(AmbientOcclusionStage::ParamsLayout) == 112, "Must match GTAOParams in Common/GTAO.hlsli");

	// XeGTAO's defaults (XeGTAO.h, GTAOSettings). The radius is in metres, the world's units.
	constexpr float k_effectRadius = 0.5f;
	constexpr float k_effectFalloffRange = 0.615f;
	constexpr float k_radiusMultiplier = 1.457f;
	constexpr float k_finalValuePower = 2.2f;
	constexpr float k_sampleDistributionPower = 2.0f;
	constexpr float k_thinOccluderCompensation = 0.0f;
	constexpr float k_depthMIPSamplingOffset = 3.30f;
	constexpr float k_denoiseBlurBeta = 1.2f;

	// Mirrors GTAO_DEPTH_MIP_LEVELS (Common/GTAO.hlsli): Compute_GTAOPrefilterDepths writes this many, a UAV each
	constexpr U32 k_depthMipCount = 5;
	// Pixels per thread group side: the prefilter's threads take 2x2 pixels each, the other passes one
	constexpr U32 k_prefilterGroupSize = 16;
	constexpr U32 k_groupSize = 8;

	// A view's working textures, kept in its Persistent() blackboard and remade when its target changes size
	struct AmbientOcclusionState : RenderStageData<AmbientOcclusionState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "AmbientOcclusionState";

		// View-space distance, with k_depthMipCount mips
		Rendering::TextureBuffer2DHandle ViewspaceDepth;
		// The raw and once-denoised visibility, scaled to fit UNORM; ping-ponged between denoise passes
		Rendering::TextureBuffer2DHandle Working[2];
		Rendering::TextureBuffer2DHandle Edges;
		// The finished visibility, which is published
		Rendering::TextureBuffer2DHandle Visibility;
		U32 Width = 0;
		U32 Height = 0;
	};

	Rendering::TextureBuffer2DHandle CreateWorkingTexture(Rendering::ETextureFormat format, U32 width, U32 height, U32 mipLevels)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.Width = width;
		params.Height = height;
		params.MipLevels = mipLevels;
		params.SampleCount = 1;
		params.Format = format;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// Projection is a perspective (Matrix4x4::CreatePerspectiveMatrixZeroToOne), which multiplies z by -1 into w
	bool IsPerspective(const Matrix4x4& projection)
	{
		return projection.GetRow(3).Z() < -0.5f;
	}

	unsigned int DivideRoundingUp(unsigned int value, unsigned int divisor)
	{
		return (value + divisor - 1) / divisor;
	}
}

void AmbientOcclusionStage::Initialize()
{
	m_prefilterDepthsShader = LoadComputeShader("Compute_GTAOPrefilterDepths");
	m_mainShader = LoadComputeShader("Compute_GTAOMain");
	m_denoiseShader = LoadComputeShader("Compute_GTAODenoise");

	m_paramsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ParamsLayout), 16, 1);

	Rendering::GFXTextureBufferParams whiteParams = { 0 };
	whiteParams.bIsShaderResource = true;
	whiteParams.bIsRenderTarget = true;
	whiteParams.Width = 1;
	whiteParams.Height = 1;
	whiteParams.MipLevels = 1;
	whiteParams.SampleCount = 1;
	whiteParams.Format = Rendering::ETextureFormat::RGBA8_UNORM;
	m_white = Rendering::Renderer::CreateTextureBuffer2D(whiteParams);
	Rendering::Renderer::ClearRenderTarget(m_white, Vector4D(1.0f, 1.0f, 1.0f, 1.0f));
}

void AmbientOcclusionStage::Setup(RenderStageBuilder& builder)
{
	m_sceneDepthInput = builder.Reads<SceneDepthData>();
	m_ambientOcclusionOutput = builder.Writes<AmbientOcclusionData>();
}

void AmbientOcclusionStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const Matrix4x4& projection = view.Camera.Projection;

	// Uniform lighting has no indirect light to occlude, and without a perspective there's no way back from depth
	if (view.AmbientOcclusion == EAmbientOcclusion::Off || view.Lighting != ERenderViewLighting::Scene || !IsPerspective(projection))
	{
		AmbientOcclusionData white;
		white.Visibility = m_white;
		white.Unoccluded = m_white;
		m_ambientOcclusionOutput.Publish(context, white);
		return;
	}

	const Rendering::RenderTargetTexture& renderTarget = *view.Target;
	AmbientOcclusionState& state = context.Persistent().GetOrCreate<AmbientOcclusionState>([]() { return AmbientOcclusionState(); });
	if (state.Width != renderTarget.GetWidth() || state.Height != renderTarget.GetHeight())
	{
		state.Width = renderTarget.GetWidth();
		state.Height = renderTarget.GetHeight();
		state.ViewspaceDepth = CreateWorkingTexture(Rendering::ETextureFormat::R32_FLOAT, state.Width, state.Height, k_depthMipCount);
		state.Working[0] = CreateWorkingTexture(Rendering::ETextureFormat::R8_UNORM, state.Width, state.Height, 1);
		state.Working[1] = CreateWorkingTexture(Rendering::ETextureFormat::R8_UNORM, state.Width, state.Height, 1);
		state.Edges = CreateWorkingTexture(Rendering::ETextureFormat::R8_UNORM, state.Width, state.Height, 1);
		state.Visibility = CreateWorkingTexture(Rendering::ETextureFormat::R8_UNORM, state.Width, state.Height, 1);
	}

	const AmbientOcclusionSettings settings = GetAmbientOcclusionSettings(view.AmbientOcclusion);
	const U32 width = static_cast<U32>(view.ViewportSize.X());
	const U32 height = static_cast<U32>(view.ViewportSize.Y());

	// The projection's rows, as it multiplies a column vector: x and y scale by 1 / tan(half the field of view), and
	// depth = (A z + B) / -z, which turns back into the distance as -B / (-A - depth)
	const Vector4D row0 = projection.GetRow(0);
	const Vector4D row1 = projection.GetRow(1);
	const Vector4D row2 = projection.GetRow(2);

	ParamsLayout params = {};
	params.ViewportSize[0] = view.ViewportSize.X();
	params.ViewportSize[1] = view.ViewportSize.Y();
	params.ViewportPixelSize[0] = 1.0f / params.ViewportSize[0];
	params.ViewportPixelSize[1] = 1.0f / params.ViewportSize[1];
	params.DepthUnpackConsts[0] = -row2.W();
	params.DepthUnpackConsts[1] = -row2.Z();
	params.CameraTanHalfFOV[0] = 1.0f / row0.X();
	params.CameraTanHalfFOV[1] = 1.0f / row1.Y();
	params.NDCToViewMul[0] = params.CameraTanHalfFOV[0] * 2.0f;
	params.NDCToViewMul[1] = params.CameraTanHalfFOV[1] * -2.0f;
	params.NDCToViewAdd[0] = params.CameraTanHalfFOV[0] * -1.0f;
	params.NDCToViewAdd[1] = params.CameraTanHalfFOV[1] * 1.0f;
	params.NDCToViewMul_x_PixelSize[0] = params.NDCToViewMul[0] * params.ViewportPixelSize[0];
	params.NDCToViewMul_x_PixelSize[1] = params.NDCToViewMul[1] * params.ViewportPixelSize[1];
	params.EffectRadius = k_effectRadius;
	params.EffectFalloffRange = k_effectFalloffRange;
	params.RadiusMultiplier = k_radiusMultiplier;
	params.FinalValuePower = k_finalValuePower;
	params.DenoiseBlurBeta = k_denoiseBlurBeta;
	params.SampleDistributionPower = k_sampleDistributionPower;
	params.ThinOccluderCompensation = k_thinOccluderCompensation;
	params.DepthMIPSamplingOffset = k_depthMIPSamplingOffset;
	params.SliceCount = static_cast<float>(settings.SliceCount);
	params.StepsPerSlice = static_cast<float>(settings.StepsPerSlice);
	params.FinalApply = settings.DenoisePasses == 1 ? 1.0f : 0.0f;

	Rendering::Renderer::Begin("AmbientOcclusionStage");

	// The depth is still bound as the prepass's target, and a texture can't be read while it's written
	Rendering::Renderer::ClearRenderTargets();

	Rendering::Renderer::UploadDataToBuffer<ParamsLayout>(m_paramsBuffer, &params);
	Rendering::Renderer::SetConstantBufferCS(m_paramsBuffer, 0);

	// 1. Depth to view-space distance, and its mips
	Rendering::Renderer::SetComputeShader(m_prefilterDepthsShader->GetPlatformObject());
	Rendering::Renderer::SetTextureResourceCS(m_sceneDepthInput.Get(context).Depth, 0);
	for (U32 mip = 0; mip < k_depthMipCount; ++mip)
	{
		Rendering::Renderer::SetUnorderedAccess(state.ViewspaceDepth, mip, mip);
	}
	Rendering::Renderer::Dispatch(DivideRoundingUp(width, k_prefilterGroupSize), DivideRoundingUp(height, k_prefilterGroupSize));
	for (U32 mip = 0; mip < k_depthMipCount; ++mip)
	{
		Rendering::Renderer::UnsetUnorderedAccess(mip);
	}
	// Unbound so the forward pass can bind it as its depth target again
	Rendering::Renderer::UnsetTextureResourceCS(0);

	// 2. The visibility and the depth edges
	Rendering::Renderer::SetComputeShader(m_mainShader->GetPlatformObject());
	Rendering::Renderer::SetTextureResourceCS(state.ViewspaceDepth, 0);
	Rendering::Renderer::SetUnorderedAccess(state.Working[0], 0);
	Rendering::Renderer::SetUnorderedAccess(state.Edges, 1);
	Rendering::Renderer::Dispatch(DivideRoundingUp(width, k_groupSize), DivideRoundingUp(height, k_groupSize));
	Rendering::Renderer::UnsetUnorderedAccess(0);
	Rendering::Renderer::UnsetUnorderedAccess(1);
	Rendering::Renderer::UnsetTextureResourceCS(0);

	// 3. Denoise: between the working textures, the last pass into the published one
	Rendering::Renderer::SetComputeShader(m_denoiseShader->GetPlatformObject());
	Rendering::Renderer::SetTextureResourceCS(state.Edges, 1);
	U32 source = 0;
	for (int pass = 0; pass < settings.DenoisePasses; ++pass)
	{
		const bool isLastPass = pass == settings.DenoisePasses - 1;
		if (pass > 0)
		{
			// The params are copied as they're recorded, so the earlier passes keep theirs
			params.FinalApply = isLastPass ? 1.0f : 0.0f;
			Rendering::Renderer::UploadDataToBuffer<ParamsLayout>(m_paramsBuffer, &params);
		}

		const Rendering::TextureBuffer2DHandle& destination = isLastPass ? state.Visibility : state.Working[1 - source];
		Rendering::Renderer::SetTextureResourceCS(state.Working[source], 0);
		Rendering::Renderer::SetUnorderedAccess(destination, 0);
		Rendering::Renderer::Dispatch(DivideRoundingUp(width, k_groupSize), DivideRoundingUp(height, k_groupSize));
		Rendering::Renderer::UnsetUnorderedAccess(0);
		Rendering::Renderer::UnsetTextureResourceCS(0);
		source = 1 - source;
	}
	Rendering::Renderer::UnsetTextureResourceCS(1);

	AmbientOcclusionData ambientOcclusion;
	ambientOcclusion.Visibility = state.Visibility;
	ambientOcclusion.Unoccluded = m_white;
	m_ambientOcclusionOutput.Publish(context, ambientOcclusion);

	Rendering::Renderer::End();
}
