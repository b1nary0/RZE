#include <Graphics/RenderStages/PostProcessRenderStage.h>

#include <Graphics/FullScreenPass.h>
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

	// A view's eye-adaptation history, kept in its Persistent() blackboard so every view adapts on its own
	struct EyeAdaptationState : RenderStageData<EyeAdaptationState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "EyeAdaptationState";

		// 1x1 adapted log-luminance (r) and has-been-measured flag (g). Ping-ponged: read last frame's, write this frame's.
		Rendering::TextureBuffer2DHandle Adapted[2];
		U32 AdaptedIndex = 0;
		// The first frame's history is uninitialized texture memory
		bool NeedsReset = true;
	};

	// A view's tonemapped image before anti-aliasing, for views that anti-alias, kept in its Persistent() blackboard and
	// remade when the view's target changes size
	struct TonemapTargetState : RenderStageData<TonemapTargetState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "TonemapTargetState";

		Rendering::TextureBuffer2DHandle Tonemapped;
		U32 Width = 0;
		U32 Height = 0;
	};

	Rendering::TextureBuffer2DHandle CreateTonemappedTexture(U32 width, U32 height)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsRenderTarget = true;
		params.bIsShaderResource = true;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RGBA8_UNORM;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

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
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}
}

void PostProcessRenderStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout = {};

	m_vertexShader = LoadVertexShader("Vertex_RenderTargetQuad", inputLayout);

	m_luminanceShader = LoadPixelShader("Pixel_LuminanceLog");

	m_adaptShader = LoadPixelShader("Pixel_ExposureAdapt");

	m_tonemapShader = LoadPixelShader("Pixel_Tonemap");

	Rendering::GFXTextureBufferParams luminanceParams = { 0 };
	luminanceParams.bIsRenderTarget = true;
	luminanceParams.bIsShaderResource = true;
	luminanceParams.Width = k_luminanceSize;
	luminanceParams.Height = k_luminanceSize;
	luminanceParams.MipLevels = k_luminanceMipCount;
	luminanceParams.SampleCount = 1;
	luminanceParams.Format = Rendering::ETextureFormat::RG16_FLOAT;
	m_luminance = Rendering::Renderer::CreateTextureBuffer2D(luminanceParams);

	m_paramsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ParamsLayout), 16, 1);
}

void PostProcessRenderStage::Setup(RenderStageBuilder& builder)
{
	m_sceneColourInput = builder.Reads<SceneColourData>();
	m_tonemappedColourOutput = builder.Writes<TonemappedColourData>();
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
	params.AutoExposure = view.AutoExposure ? 1.0f : 0.0f;
	params._pad0 = 0.0f;

	Rendering::Renderer::UploadDataToBuffer<ParamsLayout>(m_paramsBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_paramsBuffer, 0);

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());

	// Measuring and adapting only matter to auto-exposure. Without it, the tonemap ignores the adapted luminance.
	if (view.AutoExposure)
	{
		const Rendering::TextureBuffer2DHandle previousAdapted = eyeAdaptation.Adapted[eyeAdaptation.AdaptedIndex];
		eyeAdaptation.AdaptedIndex = 1 - eyeAdaptation.AdaptedIndex;
		const Rendering::TextureBuffer2DHandle currentAdapted = eyeAdaptation.Adapted[eyeAdaptation.AdaptedIndex];

		// Log luminance of the scene viewport, averaged down the mip chain
		Rendering::Renderer::SetColourTarget(m_luminance);
		Rendering::Renderer::SetViewport({ static_cast<float>(k_luminanceSize), static_cast<float>(k_luminanceSize) });
		DrawFullScreenPass(m_luminanceShader, { sceneColour.Colour });
		// Unbind so the texture can be read while its mips are generated
		Rendering::Renderer::ClearRenderTargets();
		Rendering::Renderer::GenerateMips(m_luminance);

		// Eye adaptation toward the measured average
		Rendering::Renderer::SetColourTarget(currentAdapted);
		Rendering::Renderer::SetViewport({ 1.0f, 1.0f });
		DrawFullScreenPass(m_adaptShader, { m_luminance, previousAdapted });
	}

	// Expose, tonemap and encode into the 8-bit image: straight into the view's target if it doesn't anti-alias, otherwise
	// into the image AntiAliasingStage smooths into the target. No depth bound, so the quad leaves the scene depth
	// intact for overlays that depth-test against it.
	TonemappedColourData tonemapped;
	tonemapped.Depth = sceneColour.Depth;
	if (view.AntiAliasing == EAntiAliasing::Off)
	{
		tonemapped.Colour = renderTarget.GetTargetPlatformObject();
	}
	else
	{
		TonemapTargetState& state = context.Persistent().GetOrCreate<TonemapTargetState>([]() { return TonemapTargetState(); });
		if (state.Width != renderTarget.GetWidth() || state.Height != renderTarget.GetHeight())
		{
			state.Width = renderTarget.GetWidth();
			state.Height = renderTarget.GetHeight();
			state.Tonemapped = CreateTonemappedTexture(state.Width, state.Height);
		}
		tonemapped.Colour = state.Tonemapped;
	}

	Rendering::Renderer::SetViewport({ viewportSize.X(), viewportSize.Y() });
	Rendering::Renderer::SetColourTarget(tonemapped.Colour);
	DrawFullScreenPass(m_tonemapShader, { sceneColour.Colour, eyeAdaptation.Adapted[eyeAdaptation.AdaptedIndex] });

	m_tonemappedColourOutput.Publish(context, tonemapped);

	Rendering::Renderer::End();
}
