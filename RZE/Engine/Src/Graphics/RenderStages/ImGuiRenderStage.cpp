#include <StdAfx.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <Rendering/Renderer.h>

#include <imGUI/imgui_impl_dx11.h>

void ImGuiRenderStage::Initialize()
{
	// @TODO really not a huge fan of this... come back to this later
	//Rendering::Renderer::InitializeImGui();
}

void ImGuiRenderStage::Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
{
	OPTICK_EVENT();

	if (m_withEditor)
	{
		Rendering::Renderer::SetRenderTargetBackBuffer();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
