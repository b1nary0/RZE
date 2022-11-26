#include <StdAfx.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <Rendering/Renderer.h>

#include <imGUI/imgui_impl_dx11.h>

void ImGuiRenderStage::Initialize()
{
	// @TODO really not a huge fan of this... come back to this later
	//Rendering::Renderer::InitializeImGui();
}

void ImGuiRenderStage::Render(const RenderStageData& renderData)
{
	OPTICK_EVENT();
	Rendering::Renderer::Begin("ImGuiRenderStage");

	if (m_withEditor)
	{
		Rendering::Renderer::SetRenderTargetBackBuffer();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	Rendering::Renderer::End();
}
