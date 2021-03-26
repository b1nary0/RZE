#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>
#include <Diotima/Graphics/RenderTarget.h>

#include <ECS/Components/CameraComponent.h>

#include <ImGui/imgui.h>

#include <algorithm>

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
				RZE_Application::RZE().GetRenderer().SetViewportSize(mDimensions);

				Apollo::EntityHandler& entityHandler = RZE_Application::RZE().GetActiveScene().GetEntityHandler();

				Functor<void, Apollo::EntityID> cameraUpdateFunc([this, &entityHandler](Apollo::EntityID entityID) 
				{
					CameraComponent* const cameraComp = entityHandler.GetComponent<CameraComponent>(entityID);
					AssertNotNull(cameraComp);
					if (cameraComp->bIsActiveCamera)
					{
						cameraComp->AspectRatio = mDimensions.X() / mDimensions.Y();
					}
				});
				entityHandler.ForEach<CameraComponent>(cameraUpdateFunc);
			}

			Diotima::RenderTargetTexture* const pRTT = RZE_Application::RZE().GetApplication().GetRTT();
			Diotima::DX11GFXTextureBuffer2D& GFXTexture = pRTT->GetGFXTexture();

			auto clamp = [](float a, float b, float val)
			{
				if (val > b) val = b;
				else if (val < a) val = a;

				return val;
			};
			float uvbx = clamp(0.0f, 1.0f, mDimensions.X() / pRTT->GetWidth());
			float uvby = clamp(0.0f, 1.0f, mDimensions.Y() / pRTT->GetHeight());
			ImGui::Image((void*)&GFXTexture.GetResourceView(), ImVec2(mDimensions.X(), mDimensions.Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

}