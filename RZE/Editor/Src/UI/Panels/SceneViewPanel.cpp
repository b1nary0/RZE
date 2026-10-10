#include <UI/Panels/SceneViewPanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/DirectionalLightComponent.h>
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
				RZE().GetRenderEngine().GetMainView().ViewportSize = m_dimensions;

				// The editor camera doesn't exist while a scene is loading; it picks up
				// the current dimensions when it's created (EditorApp::CreateAndInitializeEditorCamera).
				GameObjectPtr cameraObject = editorApp.GetCameraObject();
				if (cameraObject != nullptr)
				{
					GameObjectComponentPtr<EditorCameraComponent> cameraComponent = cameraObject->GetComponent<EditorCameraComponent>();
					AssertNotNull(cameraComponent);
					cameraComponent->SetAspectRatio(m_dimensions.X() / m_dimensions.Y());
				}
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

				ImGui::Image((ImTextureID)(intptr_t)texture.GetTextureData(), ImVec2(GetDimensions().X(), GetDimensions().Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));

				{
					GameObjectPtr selectedGameObject = editorApp.GetSelectedObjectFromScenePanel();
					GameObjectPtr cameraObject = editorApp.GetCameraObject();
					if (selectedGameObject != nullptr && cameraObject != nullptr)
					{
						ImGuizmo::SetOrthographic(false);
						ImGuizmo::SetDrawlist();


						const Vector2D& sceneViewDims = GetDimensions();

						Vector2D sceneViewPos = GetPosition();
						sceneViewPos.SetY(sceneViewPos.Y() + cursorPos.y);

						ImGuizmo::SetRect(sceneViewPos.X(), sceneViewPos.Y(), sceneViewDims.X(), sceneViewDims.Y());

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

				// After the gizmo so it can't hide them; they're draw-only, so the gizmo still takes the clicks
				DrawObjectIcons(ImVec2(GetPosition().X(), GetPosition().Y() + cursorPos.y));

			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewPanel::DrawObjectIcons(const ImVec2& viewOrigin)
	{
		EditorApp& editorApp = static_cast<EditorApp&>(RZE().GetApplication());
		// The camera is empty while a scene loads
		GameObjectPtr cameraObject = editorApp.GetCameraObject();
		if (cameraObject == nullptr)
		{
			return;
		}

		GameObjectComponentPtr<EditorCameraComponent> cameraComponent = cameraObject->GetComponent<EditorCameraComponent>();
		const Matrix4x4 viewProjection = cameraComponent->GetProjectionMatrix() * cameraComponent->GetViewMatrix();

		const Vector2D& viewSize = GetDimensions();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// False when the point is behind the editor camera
		auto worldToScreen = [&](const Vector3D& position, ImVec2& outScreen)
		{
			const Vector4D clip = viewProjection * Vector4D(position.X(), position.Y(), position.Z(), 1.0f);
			if (clip.W() <= 0.0f)
			{
				return false;
			}

			outScreen = ImVec2(
				viewOrigin.x + (clip.X() / clip.W() * 0.5f + 0.5f) * viewSize.X(),
				viewOrigin.y + (0.5f - clip.Y() / clip.W() * 0.5f) * viewSize.Y());
			return true;
		};

		GameObjectPtr selectedGameObject = editorApp.GetSelectedObjectFromScenePanel();

		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([&](GameObjectPtr gameObject)
			{
				const Vector3D& position = gameObject->GetTransformComponent()->GetPosition();

				// The selected object's icon sits on top of its gizmo; fade it so the gizmo's centre handle shows through
				const bool isSelected = selectedGameObject != nullptr && gameObject == selectedGameObject;
				const float alpha = isSelected ? 0.4f : 1.0f;
				auto withAlpha = [alpha](int r, int g, int b, int a)
				{
					return IM_COL32(r, g, b, static_cast<int>(a * alpha));
				};
				const ImU32 outlineColour = withAlpha(0, 0, 0, 200);

				if (gameObject->GetComponent<DirectionalLightComponent>() != nullptr)
				{
					ImVec2 center;
					if (worldToScreen(position, center))
					{
						// Sun: filled disc with eight rays
						constexpr float k_discRadius = 7.0f;
						constexpr float k_rayInner = 10.0f;
						constexpr float k_rayOuter = 15.0f;
						const ImU32 sunColour = withAlpha(255, 210, 60, 255);

						drawList->AddCircleFilled(center, k_discRadius, sunColour, 16);
						drawList->AddCircle(center, k_discRadius, outlineColour, 16, 1.5f);
						for (int ray = 0; ray < 8; ++ray)
						{
							const float angle = ray * (3.14159265f / 4.0f);
							const ImVec2 dir(std::cos(angle), std::sin(angle));
							drawList->AddLine(
								ImVec2(center.x + dir.x * k_rayInner, center.y + dir.y * k_rayInner),
								ImVec2(center.x + dir.x * k_rayOuter, center.y + dir.y * k_rayOuter),
								sunColour, 2.0f);
						}
					}
				}

				GameObjectComponentPtr<CameraComponent> camera = gameObject->GetComponent<CameraComponent>();
				if (camera != nullptr)
				{
					DrawCameraFrustum(position, **camera);

					ImVec2 center;
					if (worldToScreen(position, center))
					{
						// Movie camera: body, two film reels on top, lens on the right
						constexpr float k_reelRadius = 6.0f;
						const ImU32 cameraColour = withAlpha(200, 215, 235, 255);
						const ImVec2 bodyMin(center.x - 15.0f, center.y - 6.0f);
						const ImVec2 bodyMax(center.x + 6.0f, center.y + 9.0f);
						const ImVec2 reelA(center.x - 9.0f, center.y - 12.0f);
						const ImVec2 reelB(center.x + 1.5f, center.y - 12.0f);
						const ImVec2 lensA(center.x + 6.0f, center.y + 1.5f);
						const ImVec2 lensB(center.x + 16.0f, center.y - 6.0f);
						const ImVec2 lensC(center.x + 16.0f, center.y + 9.0f);

						drawList->AddCircleFilled(reelA, k_reelRadius, cameraColour, 12);
						drawList->AddCircle(reelA, k_reelRadius, outlineColour, 12, 1.5f);
						drawList->AddCircleFilled(reelB, k_reelRadius, cameraColour, 12);
						drawList->AddCircle(reelB, k_reelRadius, outlineColour, 12, 1.5f);
						drawList->AddTriangleFilled(lensA, lensB, lensC, cameraColour);
						drawList->AddTriangle(lensA, lensB, lensC, outlineColour, 1.5f);
						drawList->AddRectFilled(bodyMin, bodyMax, cameraColour, 1.5f);
						drawList->AddRect(bodyMin, bodyMax, outlineColour, 1.5f, 0, 1.5f);
					}
				}
			}));
	}

	void SceneViewPanel::DrawCameraFrustum(const Vector3D& position, const CameraComponent& camera)
	{
		// Same basis glm::lookAt builds the camera's view matrix from
		const Vector3D forward = camera.GetForward().Normalized();
		const Vector3D right = forward.Cross(camera.GetUpDir()).Normalized();
		const Vector3D up = right.Cross(forward);

		// The aspect ratio is only set once the camera is in a scene; fall back to the scene view's
		const float aspectRatio = camera.GetAspectRatio() > 0.0f ? camera.GetAspectRatio() : GetDimensions().X() / GetDimensions().Y();
		const float tanHalfFov = std::tan(camera.GetFOV() * 0.5f * MathUtils::ToRadians);

		// Corner i is on the right if bit 0 is set and on top if bit 1 is set
		auto planeCorners = [&](float distance, Vector3D (&outCorners)[4])
		{
			const Vector3D center = position + forward * distance;
			const float halfHeight = distance * tanHalfFov;
			const float halfWidth = halfHeight * aspectRatio;
			for (int i = 0; i < 4; ++i)
			{
				outCorners[i] = center
					+ right * ((i & 1) ? halfWidth : -halfWidth)
					+ up * ((i & 2) ? halfHeight : -halfHeight);
			}
		};

		Vector3D nearCorners[4];
		Vector3D farCorners[4];
		// Scene cameras usually see ~1000 units out, which would draw as four lines running off-screen
		// with a microscopic near plane; stopping short keeps the frustum readable as a pyramid
		constexpr float k_maxFrustumDrawDistance = 10.0f;
		planeCorners(camera.GetNearCull(), nearCorners);
		planeCorners(std::min(camera.GetFarCull(), k_maxFrustumDrawDistance), farCorners);

		const Vector3D frustumColour(0.8f, 0.85f, 0.9f);
		RenderEngine& renderEngine = RZE().GetRenderEngine();

		// Walks each plane's rectangle: bottom-left, bottom-right, top-right, top-left
		constexpr int k_ring[4] = { 0, 1, 3, 2 };
		for (int i = 0; i < 4; ++i)
		{
			const int corner = k_ring[i];
			const int nextCorner = k_ring[(i + 1) % 4];
			renderEngine.DrawLine(nearCorners[corner], nearCorners[nextCorner], frustumColour);
			renderEngine.DrawLine(farCorners[corner], farCorners[nextCorner], frustumColour);
			renderEngine.DrawLine(nearCorners[corner], farCorners[corner], frustumColour);
		}
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
