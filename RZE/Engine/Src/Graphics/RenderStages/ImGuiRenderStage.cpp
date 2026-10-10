#include <StdAfx.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <Rendering/Renderer.h>

#include <imGUI/imgui_impl_dx11.h>

void ImGuiRenderStage::Initialize()
{
	// @TODO really not a huge fan of this... come back to this later
	//Rendering::Renderer::InitializeImGui();
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

	ImGui::Render();
	Rendering::Renderer::ImGuiRender();

	Rendering::Renderer::End();
}
