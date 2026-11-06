#include <Graphics/RenderStages/DebugViewStage.h>

#include <Graphics/FullScreenPass.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

void DebugViewStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout = {};
	m_vertexShader = LoadVertexShader("Vertex_RenderTargetQuad", inputLayout);

	m_ambientOcclusionShader = LoadPixelShader("Pixel_DebugAmbientOcclusion");
}

void DebugViewStage::Setup(RenderStageBuilder& builder)
{
	m_ambientOcclusionInput = builder.Reads<AmbientOcclusionData>();
	m_displayColour = builder.Modifies<DisplayColourData>();
}

void DebugViewStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	if (view.Debug != ERenderViewDebug::AmbientOcclusion)
	{
		return;
	}

	Rendering::Renderer::Begin("DebugViewStage");

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });
	Rendering::Renderer::SetColourTarget(m_displayColour.Get(context).Colour);
	DrawFullScreenPass(m_ambientOcclusionShader, { m_ambientOcclusionInput.Get(context).Visibility });

	Rendering::Renderer::End();
}
