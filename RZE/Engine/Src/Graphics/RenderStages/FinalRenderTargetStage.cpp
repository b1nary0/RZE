#include <Graphics/RenderStages/FinalRenderTargetStage.h>

#include <Graphics/FullScreenPass.h>
#include <Graphics/RenderEngine.h>

#include <Rendering/Renderer.h>

void FinalRenderTargetStage::Initialize()
{
	{
		Rendering::ShaderInputLayout inputLayout = {};

		m_vertexShader = LoadVertexShader("Vertex_RenderTargetQuad", inputLayout);
	}

	{
		m_pixelShader = LoadPixelShader("Pixel_RenderToTarget");
	}
}

void FinalRenderTargetStage::Setup(RenderStageBuilder& builder)
{
	// Copies the finished main view to the back buffer
	m_displayColourInput = builder.Reads<DisplayColourData>();
	builder.RunsIn(ERenderViewFilter::MainOnly);
}

void FinalRenderTargetStage::Render(RenderContext& context)
{
	Rendering::Renderer::Begin("FinalRenderTargetStage");

	const DisplayColourData& displayColour = m_displayColourInput.Get(context);

	Rendering::Renderer::ClearRenderTargets();
	Rendering::Renderer::SetRenderTargetBackBuffer();

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	DrawFullScreenPass(m_pixelShader, { displayColour.Colour });

	Rendering::Renderer::End();
}
