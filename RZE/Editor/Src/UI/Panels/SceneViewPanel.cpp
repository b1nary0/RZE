#include <UI/Panels/SceneViewPanel.h>

#include <RZE.h>
#include <EngineApp.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/PanelRegistry.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/DirectionalLightComponent.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/Frustum.h>
#include <Graphics/RenderEngine.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

namespace Editor
{
	REGISTER_EDITOR_PANEL(SceneViewPanel);

	PanelDesc SceneViewPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "SceneView";
		desc.DefaultDock = EPanelDock::Center;
		// The editor has nothing to show without the scene, so it stays open
		desc.Closable = false;
		desc.WindowFlags = ImGuiWindowFlags_MenuBar;
		return desc;
	}

	SceneViewPanel::SceneViewPanel()
	{
		m_gizmoState.m_currentOpMode = ImGuizmo::TRANSLATE;
		m_gizmoState.m_transformationSpace = ImGuizmo::WORLD;
	}

	void SceneViewPanel::Initialize(EditorContext& context)
	{
		RegisterInputs();
	}

	void SceneViewPanel::PushWindowStyle(PanelWindowStyle& style) const
	{
		style.Push(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	}

	void SceneViewPanel::OnSceneReset()
	{
		m_iconHits.clear();
		m_isSelecting = false;
	}

	void SceneViewPanel::DrawMenuBar(EditorContext& context)
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

		m_gizmoState.m_snapper.DisplayControls();
		m_grid.DisplayControls();
	}

	void SceneViewPanel::Draw(EditorContext& context)
	{
		EditorSelection& selection = context.GetSelection();
		SceneViewportState& viewport = context.GetSceneViewport();

		viewport.IsHovered = IsHovered();

		ImVec2 viewportPos = ImGui::GetWindowPos();
		if (viewportPos.x != m_position.X() && viewportPos.y != m_position.Y())
		{
			m_position.SetXY(viewportPos.x, viewportPos.y);
		}

		ImVec2 viewportDims = ImGui::GetContentRegionAvail();
		if (viewportDims.x != m_dimensions.X() || viewportDims.y != m_dimensions.Y())
		{
			m_dimensions.SetXY(viewportDims.x, viewportDims.y);
			viewport.Size = m_dimensions;
			RZE().GetRenderEngine().GetMainView().ViewportSize = m_dimensions;

			// The editor camera doesn't exist while a scene is loading; it picks up
			// the viewport size when it's created (EditorApp::CreateAndInitializeEditorCamera).
			GameObjectComponentPtr<EditorCameraComponent> cameraComponent = context.GetEditorCameraComponent();
			if (cameraComponent != nullptr)
			{
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
			const bool isImageHovered = ImGui::IsItemHovered();

			{
				GameObjectPtr selectedGameObject = selection.GetPrimary();
				GameObjectComponentPtr<EditorCameraComponent> cameraComponent = context.GetEditorCameraComponent();
				if (selectedGameObject != nullptr && cameraComponent != nullptr)
				{
					ImGuizmo::SetOrthographic(false);
					ImGuizmo::SetDrawlist();


					const Vector2D& sceneViewDims = GetDimensions();

					Vector2D sceneViewPos = GetPosition();
					sceneViewPos.SetY(sceneViewPos.Y() + cursorPos.y);

					ImGuizmo::SetRect(sceneViewPos.X(), sceneViewPos.Y(), sceneViewDims.X(), sceneViewDims.Y());

					const Matrix4x4& view = cameraComponent->GetViewMatrix();
					const Matrix4x4& projection = cameraComponent->GetProjectionMatrix();

					GameObjectComponentPtr<TransformComponent> transformComponent = selectedGameObject->GetComponent<TransformComponent>();

					float matrix[16];
					ImGuizmo::RecomposeMatrixFromComponents(transformComponent->GetPosition().GetValuePtr(), transformComponent->GetRotation().GetValuePtr(), transformComponent->GetScale().GetValuePtr(), matrix);

					GizmoSnapper& snapper = m_gizmoState.m_snapper;
					const float* snap = snapper.PrepareManipulate(m_gizmoState.m_currentOpMode, m_gizmoState.m_transformationSpace, ImGui::GetIO().KeyCtrl);

					ImGuizmo::Manipulate(view.GetValuePtr(), projection.GetValuePtr(), (ImGuizmo::OPERATION)m_gizmoState.m_currentOpMode, (ImGuizmo::MODE)m_gizmoState.m_transformationSpace, matrix, nullptr, snap);

					const bool isUsing = ImGuizmo::IsUsing();
					Vector3D newTranslation, newRotation, newScale;
					if (isUsing)
					{
						ImGuizmo::DecomposeMatrixToComponents(matrix, newTranslation.GetValuePtr(), newRotation.GetValuePtr(), newScale.GetValuePtr());
					}

					// Called every frame, so it sees drags start and end
					snapper.Apply(isUsing, transformComponent->GetPosition(), transformComponent->GetRotation(), newTranslation, newRotation);

					if (isUsing)
					{
						transformComponent->SetPosition(newTranslation);
						transformComponent->SetRotation(newRotation);
						transformComponent->SetScale(newScale);
					}
				}
			}

			// After the gizmo so it can't hide them; they're draw-only, so the gizmo still takes the clicks
			const ImVec2 viewOrigin(GetPosition().X(), GetPosition().Y() + cursorPos.y);
			DrawObjectIcons(context, viewOrigin);

			// After the icons so this frame's icon rects are picked against
			HandleSelectionInput(context, viewOrigin, isImageHovered);

			// Last, so the grid's lines follow the opaque debug lines (frustums) and its depth writes can't hide them
			GameObjectPtr cameraObject = context.GetEditorCamera();
			if (cameraObject != nullptr)
			{
				m_grid.Draw(cameraObject->GetTransformComponent()->GetPosition(), m_gizmoState.m_snapper.GetSnapSize());
			}
		}
	}

	void SceneViewPanel::DrawObjectIcons(const EditorContext& context, const ImVec2& viewOrigin)
	{
		m_iconHits.clear();

		// The camera is empty while a scene loads
		GameObjectComponentPtr<EditorCameraComponent> cameraComponent = context.GetEditorCameraComponent();
		if (cameraComponent == nullptr)
		{
			return;
		}

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

		GameObjectPtr selectedGameObject = context.GetSelection().GetPrimary();

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
							const float angle = ray * (MathUtils::Pi / 4.0f);
							const ImVec2 dir(std::cos(angle), std::sin(angle));
							drawList->AddLine(
								ImVec2(center.x + dir.x * k_rayInner, center.y + dir.y * k_rayInner),
								ImVec2(center.x + dir.x * k_rayOuter, center.y + dir.y * k_rayOuter),
								sunColour, 2.0f);
						}

						m_iconHits.push_back({ gameObject, Vector2D(center.x - k_rayOuter, center.y - k_rayOuter), Vector2D(center.x + k_rayOuter, center.y + k_rayOuter) });
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

						// From the reels' tops to the lens's right edge
						m_iconHits.push_back({ gameObject, Vector2D(bodyMin.x, reelA.y - k_reelRadius), Vector2D(lensB.x, bodyMax.y) });
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
		// Scene cameras usually see ~1000 m out, which would draw as four lines running off-screen
		// with a microscopic near plane; stopping short keeps the frustum readable as a pyramid.
		// Metres, sized like a physical camera; it was 10 units before scenes were rescaled to metres,
		// which in the rescaled scenes is about this (Sponza's 0.15 m per unit).
		constexpr float k_maxFrustumDrawDistance = 1.5f;
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

	void SceneViewPanel::HandleSelectionInput(EditorContext& context, const ImVec2& viewOrigin, bool isImageHovered)
	{
		EditorSelection& selection = context.GetSelection();

		// How far the mouse can move before a click becomes a marquee
		constexpr float k_marqueeThreshold = 4.0f;

		const ImGuiIO& io = ImGui::GetIO();

		// The gizmo takes clicks on its handles, and the other buttons drive the camera
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isImageHovered && !ImGuizmo::IsOver()
			&& !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
		{
			m_isSelecting = true;
			m_selectStart = Vector2D(io.MousePos.x, io.MousePos.y);
		}

		if (!m_isSelecting)
		{
			return;
		}

		// The marquee is kept to the scene image so it maps inside the camera's view
		const ImVec2 viewMax(viewOrigin.x + GetDimensions().X(), viewOrigin.y + GetDimensions().Y());
		const ImVec2 start(m_selectStart.X(), m_selectStart.Y());
		const ImVec2 end(MathUtils::Clampf(io.MousePos.x, viewOrigin.x, viewMax.x), MathUtils::Clampf(io.MousePos.y, viewOrigin.y, viewMax.y));
		const ImVec2 rectMin(std::min(start.x, end.x), std::min(start.y, end.y));
		const ImVec2 rectMax(std::max(start.x, end.x), std::max(start.y, end.y));
		const bool isMarquee = rectMax.x - rectMin.x > k_marqueeThreshold || rectMax.y - rectMin.y > k_marqueeThreshold;

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			if (isMarquee)
			{
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(rectMin, rectMax, IM_COL32(90, 150, 255, 40));
				drawList->AddRect(rectMin, rectMax, IM_COL32(90, 150, 255, 220));
			}
			return;
		}

		m_isSelecting = false;

		const bool isAdditive = io.KeyCtrl;

		if (isMarquee)
		{
			const std::vector<GameObjectPtr> gameObjects = GatherObjectsInRect(context, viewOrigin, rectMin, rectMax);
			if (isAdditive)
			{
				selection.AddToSelection(gameObjects);
			}
			else
			{
				selection.SetSelection(gameObjects);
			}
			return;
		}

		// Icons draw over the meshes, so they're tested first
		GameObjectPtr icon = PickIcon(start);
		if (icon != nullptr)
		{
			ApplyClick(selection, icon, isAdditive);
			return;
		}

		// Meshes are picked on the GPU, so the click is applied when the result arrives
		const U32 pickX = static_cast<U32>(std::min(start.x - viewOrigin.x, GetDimensions().X() - 1.0f));
		const U32 pickY = static_cast<U32>(std::min(start.y - viewOrigin.y, GetDimensions().Y() - 1.0f));
		context.GetScenePicker().RequestPick(pickX, pickY, ScenePicker::ResultCallback([this, &selection, isAdditive](GameObjectPtr hitObject)
			{
				ApplyClick(selection, hitObject, isAdditive);
			}));
	}

	void SceneViewPanel::ApplyClick(EditorSelection& selection, GameObjectPtr hitObject, bool isAdditive)
	{
		if (hitObject == nullptr)
		{
			if (!isAdditive)
			{
				selection.Clear();
			}
		}
		else if (isAdditive)
		{
			selection.Toggle(hitObject);
		}
		else
		{
			selection.Select(hitObject);
		}
	}

	std::vector<GameObjectPtr> SceneViewPanel::GatherObjectsInRect(const EditorContext& context, const ImVec2& viewOrigin, const ImVec2& rectMin, const ImVec2& rectMax) const
	{
		std::vector<GameObjectPtr> gameObjects;

		for (const IconHit& iconHit : m_iconHits)
		{
			const float centerX = (iconHit.m_min.X() + iconHit.m_max.X()) * 0.5f;
			const float centerY = (iconHit.m_min.Y() + iconHit.m_max.Y()) * 0.5f;
			if (centerX >= rectMin.x && centerX <= rectMax.x && centerY >= rectMin.y && centerY <= rectMax.y)
			{
				gameObjects.push_back(iconHit.m_gameObject);
			}
		}

		GameObjectComponentPtr<EditorCameraComponent> cameraComponent = context.GetEditorCameraComponent();
		if (cameraComponent == nullptr)
		{
			return gameObjects;
		}

		// The rect in NDC; screen y runs down, NDC y runs up
		const Vector2D& viewSize = GetDimensions();
		const float ndcMinX = (rectMin.x - viewOrigin.x) / viewSize.X() * 2.0f - 1.0f;
		const float ndcMaxX = (rectMax.x - viewOrigin.x) / viewSize.X() * 2.0f - 1.0f;
		const float ndcMinY = 1.0f - (rectMax.y - viewOrigin.y) / viewSize.Y() * 2.0f;
		const float ndcMaxY = 1.0f - (rectMin.y - viewOrigin.y) / viewSize.Y() * 2.0f;

		const Matrix4x4 rectProjection = Matrix4x4::CreateSubRectProjection(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY);
		const Frustum frustum(rectProjection * cameraComponent->GetProjectionMatrix() * cameraComponent->GetViewMatrix());

		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([&](GameObjectPtr gameObject)
			{
				GameObjectComponentPtr<RenderComponent> renderComponent = gameObject->GetComponent<RenderComponent>();
				if (renderComponent == nullptr)
				{
					return;
				}

				RenderObjectPtr renderObject = renderComponent->GetRenderObject();
				if (renderObject == nullptr)
				{
					return;
				}

				// Submesh boxes fit tighter than the whole object's box
				for (const RenderObject::SubMeshBounds& bounds : renderObject->GetSubMeshBounds())
				{
					if (frustum.Intersects(bounds.Min, bounds.Max))
					{
						gameObjects.push_back(gameObject);
						return;
					}
				}
			}));

		return gameObjects;
	}

	GameObjectPtr SceneViewPanel::PickIcon(const ImVec2& point) const
	{
		// Later icons draw on top of earlier ones
		for (auto it = m_iconHits.rbegin(); it != m_iconHits.rend(); ++it)
		{
			if (point.x >= it->m_min.X() && point.x <= it->m_max.X() && point.y >= it->m_min.Y() && point.y <= it->m_max.Y())
			{
				return it->m_gameObject;
			}
		}

		return GameObjectPtr();
	}

	void SceneViewPanel::RegisterInputs()
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
					case Win32KeyCode::Key_X:
						m_gizmoState.m_snapper.ToggleEnabled();
						break;
					}
				}
			});
		inputHandler.BindAction(Win32KeyCode::Key_W, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_E, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_R, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_Q, EButtonState::ButtonState_Pressed, keyFunc);
		inputHandler.BindAction(Win32KeyCode::Key_X, EButtonState::ButtonState_Pressed, keyFunc);
	}
}
