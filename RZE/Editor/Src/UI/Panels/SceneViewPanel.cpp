#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
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

		// @TODO The pattern here should be to register with the InputHandler at start
		// and unregister when going bye bye
		Temp_RegisterInputs();
	}

	void SceneViewPanel::Display()
	{
		EditorApp& editorApp = static_cast<EditorApp&>(RZE().GetApplication());

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("SceneView", (bool*)1, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::RadioButton("Translate", m_gizmoState.m_currentOpMode == ImGuizmo::TRANSLATE))
				{
					m_gizmoState.m_currentOpMode = ImGuizmo::TRANSLATE;
				}

				if (ImGui::RadioButton("Rotate", m_gizmoState.m_currentOpMode == ImGuizmo::ROTATE))
				{
					m_gizmoState.m_currentOpMode = ImGuizmo::ROTATE;
				}

				if (ImGui::RadioButton("Scale", m_gizmoState.m_currentOpMode == ImGuizmo::SCALE))
				{
					m_gizmoState.m_currentOpMode = ImGuizmo::SCALE;
				}

				if (m_gizmoState.m_currentOpMode != ImGuizmo::SCALE)
				{
					const float cursorPos = ImGui::GetCursorPosX();
					const float controlSpacing = 25.f;
					ImGui::SetCursorPosX(cursorPos + controlSpacing);
					if (ImGui::RadioButton("Local", m_gizmoState.m_transformationSpace == ImGuizmo::LOCAL))
					{
						m_gizmoState.m_transformationSpace = ImGuizmo::LOCAL;
					}
					if (ImGui::RadioButton("World", m_gizmoState.m_transformationSpace == ImGuizmo::WORLD))
					{
						m_gizmoState.m_transformationSpace = ImGuizmo::WORLD;
					}
				}

				ImGui::EndMenuBar();
			}

			m_isFocused = ImGui::IsWindowFocused();
			m_isHovered = ImGui::IsWindowHovered();

			ImVec2 viewportPos = ImGui::GetWindowPos();
			if (viewportPos.x != m_position.X() && viewportPos.y != m_position.Y())
			{
				m_position.SetXY(viewportPos.x, viewportPos.y);
			}

			ImVec2 viewportDims = ImGui::GetContentRegionAvail();
			if (viewportDims.x != m_dimensions.X() || viewportDims.y != m_dimensions.Y())
			{
				m_dimensions.SetXY(viewportDims.x, viewportDims.y);
				RZE().GetRenderEngine().SetViewportSize(m_dimensions);

				// @TODO lazy josh to future josh just use object cached on EditorApp
				GameObjectPtr cameraObject = editorApp.GetCameraObject();
				GameObjectComponentPtr<EditorCameraComponent> cameraComponent = cameraObject->GetComponent<EditorCameraComponent>();
				AssertNotNull(cameraComponent);
				cameraComponent->SetAspectRatio(m_dimensions.X() / m_dimensions.Y());
			}

			Rendering::RenderTargetTexture* const pRTT = RZE().GetApplication().GetRTT();
			Rendering::TextureBuffer2DHandle texture = pRTT->GetTargetPlatformObject();

			if (texture.IsValid())
			{
				auto clamp = [](float a, float b, float val)
					{
						if (val > b) val = b;
						else if (val < a) val = a;

						return val;
					};
				float uvbx = clamp(0.0f, 1.0f, m_dimensions.X() / pRTT->GetWidth());
				float uvby = clamp(0.0f, 1.0f, m_dimensions.Y() / pRTT->GetHeight());

				ImVec2 cursorPos = ImGui::GetCursorPos();

				ImGui::Image(texture.GetTextureData(), ImVec2(GetDimensions().X(), GetDimensions().Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));

				{
					GameObjectPtr selectedGameObject = editorApp.GetSelectedObjectFromScenePanel();
					if (selectedGameObject != nullptr)
					{
						ImGuizmo::SetOrthographic(false);
						ImGuizmo::SetDrawlist();


						const Vector2D& sceneViewDims = GetDimensions();

						Vector2D sceneViewPos = GetPosition();
						sceneViewPos.SetY(sceneViewPos.Y() + cursorPos.y);

						ImGuizmo::SetRect(sceneViewPos.X(), sceneViewPos.Y(), sceneViewDims.X(), sceneViewDims.Y());

						GameObjectPtr cameraObject = RZE().GetActiveScene().FindGameObjectByName("EditorCam");
						AssertNotNull(cameraObject);

						GameObjectComponentPtr<EditorCameraComponent> cameraComponent = cameraObject->GetComponent<EditorCameraComponent>();

						const Matrix4x4& view = cameraComponent->GetViewMatrix();
						const Matrix4x4& projection = cameraComponent->GetProjectionMatrix();

						GameObjectComponentPtr<TransformComponent> transformComponent = selectedGameObject->GetComponent<TransformComponent>();

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
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewPanel::Temp_RegisterInputs()
	{
		InputHandler& inputHandler = RZE().GetInputHandler();

		Functor<void, const InputKey&> keyFunc([this, &inputHandler](const InputKey& key)
			{
				if (inputHandler.GetMouseState().GetButtonState(EMouseButton::MouseButton_Right) != EButtonState::ButtonState_Pressed)
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
					case Win32KeyCode::Key_Q:
						m_gizmoState.m_transformationSpace = (m_gizmoState.m_transformationSpace == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
						break;
					}
				}
			});
		inputHandler.BindAction(Win32KeyCode::Key_W, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_E, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_R, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_Q, EButtonState::ButtonState_Pressed, keyFunc);
	}
}
