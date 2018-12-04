#include <Widgets/SceneViewWidget.h>

#include <EditorApp.h>

#include <Diotima/Graphics/RenderTarget.h>

SceneViewWidget::SceneViewWidget(Diotima::RenderTarget* renderTarget)
	: pRTT(renderTarget)
{
}

SceneViewWidget::~SceneViewWidget()
{
}

void SceneViewWidget::Initialize()
{
	AssertNotNull(pRTT);

	pRTT->SetDimensions(1280, 720);
	pRTT->Initialize();
}

void SceneViewWidget::Display()
{
	// #TODO(Josh) Don't know about this setup
	Diotima::RenderTargetTexture* const renderTarget = static_cast<Diotima::RenderTargetTexture*>(pRTT);

	Vector2D size(static_cast<float>(pRTT->GetWidth()), static_cast<float>(pRTT->GetHeight()));
	ImGui::SetNextWindowSize(ImVec2(size.X(), size.Y() + 20)); // #TODO(Josh) Why +20?
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 20.0f), 0, ImVec2(0.5f, 0.0f)); // #TODO(Josh) Need to find solution for the magic numbers here (20.0f) which put this window below the main menu bar
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin("SceneView", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) // #TODO(Josh) Why does the window need scroll bar?
	{
		bIsFocused = ImGui::IsWindowFocused();
		bIsHovered = ImGui::IsWindowHovered();

		ImGui::Image((void*)renderTarget->GetTextureID(), ImVec2(size.X(), size.Y()), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::End();
	}
	ImGui::PopStyleVar();
}
