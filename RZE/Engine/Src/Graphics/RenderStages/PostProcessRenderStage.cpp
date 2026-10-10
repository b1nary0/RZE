#include <StdAfx.h>
#include <Graphics/RenderStages/PostProcessRenderStage.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>
#include <Rendering/Graphics/RenderTarget.h>

namespace
{
	// Power of two so every mip halves exactly and the 1x1 mip is a true average
	constexpr U32 k_luminanceSize = 256;
	constexpr U32 k_luminanceMipCount = 9; // 256 -> 1

	// The scene's log-average luminance is exposed to this. Reinhard's low-key value rather than
	// the 0.18 mid-grey: scenes here are mostly shade with some direct sun (Sponza's log-average
	// is ~0.015), and 0.18 pushes the sunlit parts into clipping. Per-scene EV adjusts from here.
	constexpr float k_keyValue = 0.045f;
	// Per second; about 1.5s to settle after a big change in brightness
	constexpr float k_adaptationRate = 1.5f;
	// Keeps near-black or blown-out views from being pushed to extremes (-3 to +4 EV)
	constexpr float k_minExposure = 0.125f;
	constexpr float k_maxExposure = 16.0f;

	// Must match the register(tN) slots in the post-process shaders
	constexpr U32 k_sourceSlot = 0;
	constexpr U32 k_secondarySlot = 1;

	// A view's eye-adaptation history, kept in its Persistent() blackboard so every view adapts on its own
	struct EyeAdaptationState : RenderStageData<EyeAdaptationState>
	{
		static constexpr const char* k_name = "EyeAdaptationState";

		// 1x1 adapted log-luminance (r) and has-been-measured flag (g). Ping-ponged: read last frame's, write this frame's.
		Rendering::TextureBuffer2DHandle Adapted[2];
		U32 AdaptedIndex = 0;
		// The first frame's history is uninitialized texture memory
		bool NeedsReset = true;
	};

	Rendering::TextureBuffer2DHandle CreateAdaptedTexture()
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsRenderTarget = true;
		params.bIsShaderResource = true;
		params.Width = 1;
		params.Height = 1;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RG16_FLOAT;
		return Rendering::Renderer::CreateTextureBuffer2D(nullptr, params);
	}
}

void PostProcessRenderStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout = {};

	m_vertexShaderResource = RZE().GetResourceHandler().LoadResource<VertexShader>(Filepath("Assets/Shaders/Vertex_RenderTargetQuad.hlsl"), "Vertex_RenderTargetQuad", inputLayout);
	AssertExpr(m_vertexShaderResource.IsValid());
	m_vertexShader = RZE().GetResourceHandler().GetResource<VertexShader>(m_vertexShaderResource);

	m_luminanceShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_LuminanceLog.hlsl"), "Pixel_LuminanceLog");
	AssertExpr(m_luminanceShaderResource.IsValid());
	m_luminanceShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_luminanceShaderResource);

	m_adaptShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_ExposureAdapt.hlsl"), "Pixel_ExposureAdapt");
	AssertExpr(m_adaptShaderResource.IsValid());
	m_adaptShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_adaptShaderResource);

	m_tonemapShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_Tonemap.hlsl"), "Pixel_Tonemap");
	AssertExpr(m_tonemapShaderResource.IsValid());
	m_tonemapShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_tonemapShaderResource);

	Rendering::GFXTextureBufferParams luminanceParams = { 0 };
	luminanceParams.bIsRenderTarget = true;
	luminanceParams.bIsShaderResource = true;
	luminanceParams.Width = k_luminanceSize;
	luminanceParams.Height = k_luminanceSize;
	luminanceParams.MipLevels = k_luminanceMipCount;
	luminanceParams.SampleCount = 1;
	luminanceParams.Format = Rendering::ETextureFormat::RG16_FLOAT;
	m_luminance = Rendering::Renderer::CreateTextureBuffer2D(nullptr, luminanceParams);

	m_paramsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ParamsLayout), 16, 1);
}

void PostProcessRenderStage::Setup(RenderStageBuilder& builder)
{
	m_sceneColourInput = builder.Reads<SceneColourData>();
	m_displayColourOutput = builder.Writes<DisplayColourData>();
}

void PostProcessRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const SceneColourData& sceneColour = m_sceneColourInput.Get(context);
	const Rendering::RenderTargetTexture& renderTarget = *view.Target;
	const Vector2D& viewportSize = view.ViewportSize;

	EyeAdaptationState& eyeAdaptation = context.Persistent().GetOrCreate<EyeAdaptationState>([]()
	{
		EyeAdaptationState state;
		state.Adapted[0] = CreateAdaptedTexture();
		state.Adapted[1] = CreateAdaptedTexture();
		return state;
	});
	// Secondary views (camera previews) expose instantly instead of adapting over time
	const bool resetAdaptation = eyeAdaptation.NeedsReset || !view.IsMainView();
	eyeAdaptation.NeedsReset = false;

	Rendering::Renderer::Begin("PostProcessRenderStage");

	ParamsLayout params;
	params.ViewportScale[0] = viewportSize.X() / static_cast<float>(renderTarget.GetWidth());
	params.ViewportScale[1] = viewportSize.Y() / static_cast<float>(renderTarget.GetHeight());
	params.DeltaTime = context.DeltaTime;
	params.AdaptationRate = k_adaptationRate;
	params.MinExposure = k_minExposure;
	params.MaxExposure = k_maxExposure;
	params.ExposureCompensation = view.ExposureCompensation;
	params.KeyValue = k_keyValue;
	params.LuminanceMip = static_cast<float>(k_luminanceMipCount - 1);
	params.Reset = resetAdaptation ? 1.0f : 0.0f;
	params._pad0[0] = 0.0f;
	params._pad0[1] = 0.0f;

	Rendering::Renderer::UploadDataToBuffer<ParamsLayout>(m_paramsBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_paramsBuffer, 0);

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());

	// Log luminance of the scene viewport, averaged down the mip chain
	{
		Rendering::Renderer::SetColourTarget(m_luminance);
		Rendering::Renderer::SetViewport({ static_cast<float>(k_luminanceSize), static_cast<float>(k_luminanceSize), 0.0f, 1.0f, 0.0f, 0.0f });
		Rendering::Renderer::SetPixelShader(m_luminanceShader->GetPlatformObject());
		Rendering::Renderer::SetTextureResource(sceneColour.Colour, k_sourceSlot);

		Rendering::Renderer::DrawFullScreenQuad();

		Rendering::Renderer::UnsetTextureResource(k_sourceSlot);
		// Unbind so the texture can be read while its mips are generated
		Rendering::Renderer::SetRenderTarget(nullptr);
		Rendering::Renderer::GenerateMips(m_luminance);
	}

	const Rendering::TextureBuffer2DHandle previousAdapted = eyeAdaptation.Adapted[eyeAdaptation.AdaptedIndex];
	eyeAdaptation.AdaptedIndex = 1 - eyeAdaptation.AdaptedIndex;
	const Rendering::TextureBuffer2DHandle currentAdapted = eyeAdaptation.Adapted[eyeAdaptation.AdaptedIndex];

	// Eye adaptation toward the measured average
	{
		Rendering::Renderer::SetColourTarget(currentAdapted);
		Rendering::Renderer::SetViewport({ 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f });
		Rendering::Renderer::SetPixelShader(m_adaptShader->GetPlatformObject());
		Rendering::Renderer::SetTextureResource(m_luminance, k_sourceSlot);
		Rendering::Renderer::SetTextureResource(previousAdapted, k_secondarySlot);

		Rendering::Renderer::DrawFullScreenQuad();

		Rendering::Renderer::UnsetTextureResource(k_sourceSlot);
		Rendering::Renderer::UnsetTextureResource(k_secondarySlot);
	}

	// Expose, tonemap and encode into the 8-bit target. No depth bound, so the quad leaves the
	// scene depth intact for overlays that depth-test against it.
	DisplayColourData displayColour;
	displayColour.Colour = renderTarget.GetTargetPlatformObject();
	displayColour.Depth = sceneColour.Depth;
	{
		Rendering::Renderer::SetColourTarget(displayColour.Colour);
		Rendering::Renderer::SetViewport({ viewportSize.X(), viewportSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f });
		Rendering::Renderer::SetPixelShader(m_tonemapShader->GetPlatformObject());
		Rendering::Renderer::SetTextureResource(sceneColour.Colour, k_sourceSlot);
		Rendering::Renderer::SetTextureResource(currentAdapted, k_secondarySlot);

		Rendering::Renderer::DrawFullScreenQuad();

		Rendering::Renderer::UnsetTextureResource(k_sourceSlot);
		Rendering::Renderer::UnsetTextureResource(k_secondarySlot);
	}

	m_displayColourOutput.Publish(context, displayColour);

	Rendering::Renderer::End();
}
