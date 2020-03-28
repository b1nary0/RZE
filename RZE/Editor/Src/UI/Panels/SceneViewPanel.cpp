#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Diotima/Graphics/RenderTarget.h>

#include <ImGui/imgui.h>

namespace Editor
{

	void SceneViewPanel::Display()
	{
		Diotima::RenderTargetTexture* const pRTT = RZE_Application::RZE().GetApplication().GetRTT();
		Diotima::DX11GFXTextureBuffer2D& GFXTexture = pRTT->GetGFXTexture();

		ImGui::SetNextWindowSize(ImVec2(1024, 576 + 20)); // #TODO(Josh) Why +20? -- Probably because of ImGui menu bar?
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 20.0f), 0, ImVec2(0.5f, 0.0f)); // #TODO(Josh) Need to find solution for the magic numbers here (20.0f) which put this window below the main menu bar
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("SceneView", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) // #TODO(Josh) Why does the window need scroll bar?
		{
			ImGui::Image((void*)&GFXTexture.GetResourceView(), ImVec2(static_cast<float>(pRTT->GetWidth()), static_cast<float>(pRTT->GetHeight())), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

			ImGui::End();
		}
		ImGui::PopStyleVar();
	}

}