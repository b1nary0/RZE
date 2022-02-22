#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

namespace Editor
{
	SceneViewPanel::SceneViewPanel()
	{
		m_gizmoState.m_currentOpMode = ImGuizmo::TRANSLATE;
		m_gizmoState.m_transformationSpace = ImGuizmo::WORLD;

		Temp_RegisterInputs();
	}

	void SceneViewPanel::Display()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("SceneView", (bool*)1, ImGuiWindowFlags_NoScrollbar)) // #TODO(Josh) Why does the window need scroll bar?
		{
			m_isFocused = ImGui::IsWindowFocused();
			m_isHovered = ImGui::IsWindowHovered();

			ImVec2 viewportPos = ImGui::GetWindowPos();
			if (viewportPos.x != m_position.X() && viewportPos.y != m_position.Y())
			{
				m_position.SetXY(viewportPos.x, viewportPos.y);
			}

			ImVec2 viewportDims = ImGui::GetWindowSize();
			if (viewportDims.x != m_dimensions.X() || viewportDims.y != m_dimensions.Y())
			{
				m_dimensions.SetXY(viewportDims.x, viewportDims.y);
				RZE::GetRenderEngine().SetViewportSize(m_dimensions);

				std::shared_ptr<GameObject> gameObject = RZE::GetActiveScene().FindGameObjectByName("Camera");
				AssertNotNull(gameObject);
				CameraComponent* const cameraComponent = gameObject->GetComponent<CameraComponent>();
				AssertNotNull(cameraComponent);
				cameraComponent->SetAspectRatio(m_dimensions.X() / m_dimensions.Y());
			}

			Rendering::RenderTargetTexture* const pRTT = RZE_Application::RZE().GetApplication().GetRTT();
			Rendering::TextureBuffer2DHandle texture = pRTT->GetTargetPlatformObject();

			auto clamp = [](float a, float b, float val)
			{
				if (val > b) val = b;
				else if (val < a) val = a;

				return val;
			};
			float uvbx = clamp(0.0f, 1.0f, m_dimensions.X() / pRTT->GetWidth());
			float uvby = clamp(0.0f, 1.0f, m_dimensions.Y() / pRTT->GetHeight());
			ImGui::Image(texture.GetTextureData(), ImVec2(m_dimensions.X(), m_dimensions.Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));

			{
				EditorApp& editorApp = static_cast<EditorApp&>(RZE_Application::RZE().GetApplication());
				std::shared_ptr<GameObject> selectedGameObject = editorApp.GetSelectedObjectFromScenePanel();
				if (selectedGameObject != nullptr)
				{
					ImGuizmo::SetOrthographic(false);
					ImGuizmo::SetDrawlist();

					const Vector2D& sceneViewDims = GetDimensions();
					const Vector2D& sceneViewPos = GetPosition();
					ImGuizmo::SetRect(sceneViewPos.X(), sceneViewPos.Y(), sceneViewDims.X(), sceneViewDims.Y());

					std::shared_ptr<GameObject> cameraObject = RZE::GetActiveScene().FindGameObjectByName("Camera");
					const CameraComponent* const cameraComponent = cameraObject->GetComponent<CameraComponent>();

					const Matrix4x4& view = cameraComponent->GetViewMatrix();
					const Matrix4x4& projection = cameraComponent->GetProjectionMatrix();

					TransformComponent* const transformComponent = selectedGameObject->GetComponent<TransformComponent>();
					Matrix4x4 transform = transformComponent->GetAsMat4x4();

					float* translation = &const_cast<glm::vec3&>(transformComponent->GetPosition().GetInternalVec())[0];
					float* rotation = &const_cast<glm::vec3&>(transformComponent->GetRotation().GetInternalVec())[0];
					float* scale = &const_cast<glm::vec3&>(transformComponent->GetScale().GetInternalVec())[0];

					float matrix[16];
					ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, matrix);

					ImGuizmo::Manipulate(view.GetValuePtr(), projection.GetValuePtr(), (ImGuizmo::OPERATION)m_gizmoState.m_currentOpMode, (ImGuizmo::MODE)m_gizmoState.m_transformationSpace, matrix);
					if (ImGuizmo::IsUsing())
					{
						Vector3D newTranslation, newRotation, newScale;
						ImGuizmo::DecomposeMatrixToComponents(matrix,
							&const_cast<glm::vec3&>(newTranslation.GetInternalVec())[0],
							&const_cast<glm::vec3&>(newRotation.GetInternalVec())[0],
							&const_cast<glm::vec3&>(newScale.GetInternalVec())[0]);
						
						transformComponent->SetPosition(newTranslation);
						transformComponent->SetRotation(newRotation);
						transformComponent->SetScale(newScale);
					}
				}

			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewPanel::Temp_RegisterInputs()
	{
		InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

		Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
			{
				switch (key.GetKeyCode())
				{
				case Win32KeyCode::Key_W:
					m_gizmoState.m_currentOpMode = ImGuizmo::TRANSLATE;
					break;
				case Win32KeyCode::Key_E:
					m_gizmoState.m_currentOpMode = ImGuizmo::ROTATE;
					break;
				case Win32KeyCode::Key_R:
					m_gizmoState.m_currentOpMode = ImGuizmo::SCALE;
					break;
				}
			});
		inputHandler.BindAction(Win32KeyCode::Key_W, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_E, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_R, EButtonState::ButtonState_Pressed, keyFunc);
	}
}
