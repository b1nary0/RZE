#include <Graphics/RenderStages/AntiAliasingStage.h>

#include <Graphics/FullScreenPass.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <algorithm>

void AntiAliasingStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout = {};
	m_vertexShader = LoadVertexShader("Vertex_RenderTargetQuad", inputLayout);

	m_fxaaShader = LoadPixelShader("Pixel_FXAA");

	m_fxaaParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(FXAAParamsLayout), 16, 1);
}

void AntiAliasingStage::Setup(RenderStageBuilder& builder)
{
	m_tonemappedColourInput = builder.Reads<TonemappedColourData>();
	m_displayColourOutput = builder.Writes<DisplayColourData>();
}

void AntiAliasingStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const TonemappedColourData& tonemapped = m_tonemappedColourInput.Get(context);
	const Rendering::RenderTargetTexture& renderTarget = *view.Target;
	const Vector2D& viewportSize = view.ViewportSize;

	DisplayColourData displayColour;
	displayColour.Colour = renderTarget.GetTargetPlatformObject();
	displayColour.Depth = tonemapped.Depth;

	if (view.AntiAliasing.Method != EAntiAliasingMethod::Off)
	{
		Rendering::Renderer::Begin("AntiAliasingStage");

		const FXAASettings settings = GetFXAASettings(view.AntiAliasing.Quality);
		FXAAParamsLayout params = {};
		params.ViewportScale[0] = viewportSize.X() / static_cast<float>(renderTarget.GetWidth());
		params.ViewportScale[1] = viewportSize.Y() / static_cast<float>(renderTarget.GetHeight());
		params.EdgeThreshold = settings.EdgeThreshold;
		params.EdgeThresholdMin = settings.EdgeThresholdMin;
		params.Subpixel = settings.Subpixel;
		params.SearchStepCount = static_cast<float>(settings.SearchStepCount);
		std::copy(settings.SearchSteps.begin(), settings.SearchSteps.end(), params.SearchSteps);

		Rendering::Renderer::UploadDataToBuffer<FXAAParamsLayout>(m_fxaaParamsBuffer, &params);
		Rendering::Renderer::SetConstantBufferPS(m_fxaaParamsBuffer, 0);

		Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
		Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
		Rendering::Renderer::SetViewport({ viewportSize.X(), viewportSize.Y() });
		Rendering::Renderer::SetColourTarget(displayColour.Colour);
		DrawFullScreenPass(m_fxaaShader, { tonemapped.Colour });

		Rendering::Renderer::End();
	}

	m_displayColourOutput.Publish(context, displayColour);
}
