#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <Rendering/Renderer.h>


void ImGuiRenderStage::Initialize()
{
	// Nothing to set up: the render thread creates ImGui's context and renderer (RenderThread::InitializeImGui), and
	// ImGuiPlatform its window side
}

void ImGuiRenderStage::Setup(RenderStageBuilder& builder)
{
	// ImGui::Render() ends ImGui's frame, so it can only run once per frame
	builder.RunsIn(ERenderViewFilter::MainOnly);

	if (m_withEditor)
	{
		m_displayedView = builder.Reads<DisplayColourData>();
	}
	else
	{
		m_overlayTarget = builder.Modifies<DisplayColourData>();
	}
}

void ImGuiRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();
	Rendering::Renderer::Begin("ImGuiRenderStage");

	if (m_withEditor)
	{
		Rendering::Renderer::SetRenderTargetBackBuffer();
	}
	else
	{
		Rendering::Renderer::SetColourTarget(m_overlayTarget.Get(context).Colour);
	}

	// On a frame that didn't update the UI (FrameSettings::UIFrameRate), no ImGui frame has begun, so Render() returns
	// at once and the draw data is still the last UI frame's: the same UI goes out again, around this frame's scene.
	ImGui::Render();
	Rendering::Renderer::ImGuiRender();

	Rendering::Renderer::End();
}
