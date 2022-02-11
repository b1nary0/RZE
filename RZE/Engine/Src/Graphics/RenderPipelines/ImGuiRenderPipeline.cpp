#include <StdAfx.h>
#include <Graphics/RenderPipelines/ImGuiRenderPipeline.h>

#include <Rendering/Renderer.h>

#include <imGUI/imgui_impl_dx11.h>

void ImGuiRenderPipeline::Initialize()
{
	// @TODO really not a huge fan of this... come back to this later
	Rendering::Renderer::InitializeImGui();
}

void ImGuiRenderPipeline::Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
{
	OPTICK_EVENT();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
