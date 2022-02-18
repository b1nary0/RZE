#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <ImGui/imgui.h>

namespace Editor
{

	void SceneViewPanel::Display()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("SceneView", (bool*)1, ImGuiWindowFlags_NoScrollbar)) // #TODO(Josh) Why does the window need scroll bar?
		{
			bIsFocused = ImGui::IsWindowFocused();
			bIsHovered = ImGui::IsWindowHovered();

			ImVec2 viewportDims = ImGui::GetWindowSize();
			if (viewportDims.x != mDimensions.X() || viewportDims.y != mDimensions.Y())
			{
				mDimensions.SetXY(viewportDims.x, viewportDims.y);
				RZE::GetRenderEngine().SetViewportSize(mDimensions);

				GameObject* gameObject = RZE::GetActiveScene().FindGameObjectByName("Camera");
				AssertNotNull(gameObject);
				CameraComponent* const cameraComponent = gameObject->GetComponent<CameraComponent>();
				AssertNotNull(cameraComponent);
				cameraComponent->SetAspectRatio(mDimensions.X() / mDimensions.Y());
			}

			Rendering::RenderTargetTexture* const pRTT = RZE_Application::RZE().GetApplication().GetRTT();
			Rendering::TextureBuffer2DHandle texture = pRTT->GetTargetPlatformObject();

			auto clamp = [](float a, float b, float val)
			{
				if (val > b) val = b;
				else if (val < a) val = a;

				return val;
			};
			float uvbx = clamp(0.0f, 1.0f, mDimensions.X() / pRTT->GetWidth());
			float uvby = clamp(0.0f, 1.0f, mDimensions.Y() / pRTT->GetHeight());
			ImGui::Image(texture.GetTextureData(), ImVec2(mDimensions.X(), mDimensions.Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

}