#include <UI/Panels/SceneViewPanel.h>

#include <RZE.h>
#include <EngineApp.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>

#include <Commands/SceneCommands.h>
#include <Commands/TransformCommands.h>
#include <UI/Widgets/FileTileGrid.h>

#include <Asset/AssetPaths.h>

#include <DebugUtils/DebugServices.h>

#include <EngineCore/Resources/PendingResource.h>

#include <Game/StaticMeshResource.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/DirectionalLightComponent.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderEngine.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Frustum.h>
#include <Utils/Math/Math.h>
#include <Utils/Math/Plane.h>
#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Ray.h>
#include <Utils/Math/Units.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

#include <algorithm>

namespace
{
	// Where a mesh dropped at the pick goes: on the surface under the cursor; failing that, on the ground if it's not too
	// far off; failing that, a little way out from the camera
	Vector3D GetDropPosition(const Editor::ScenePick& pick)
	{
		constexpr float k_maxGroundDistance = 100.0f;
		constexpr float k_fallbackDistance = 10.0f;

		// The editor's ground, where its grid is drawn
		const Plane ground(WorldAxes::Up(), 0.0f);

		if (pick.SurfacePosition.has_value())
		{
			return *pick.SurfacePosition;
		}
		if (const std::optional<Vector3D> groundPoint = pick.CameraRay.Intersect(ground);
			groundPoint.has_value() && (*groundPoint - pick.CameraRay.GetOrigin()).Length() <= k_maxGroundDistance)
		{
			return *groundPoint;
		}
		return pick.CameraRay.GetPoint(k_fallbackDistance);
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(SceneViewPanel);

	struct SceneViewPanel::MeshDrops
	{
		struct Drop
		{
			U64 ID = 0;
			PendingResource Mesh;
			// Set once the pick arrives
			std::optional<Vector3D> Position;
		};

		std::vector<Drop> Drops;
		// Meshes whose objects were pushed this UI frame, held until their commands have run
		std::vector<PendingResource> HandedOver;
	};

	PanelDesc SceneViewPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "SceneView";
		desc.DefaultDock = EPanelDock::Center;
		// The editor has nothing to show without the scene, so it stays open
		desc.Closable = false;
		// Panels dock around the scene, not into it
		desc.AcceptsDocking = false;
		desc.WindowFlags = ImGuiWindowFlags_MenuBar;
		// So the toolbar's controls don't crowd its edges
		desc.MenuBarPadding = 6.0f;
		desc.CentreMenuBar = true;
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
		m_gizmoState.m_drag.Reset();
		m_iconHits.clear();
		m_selectStart.reset();
		// Cancels their loads: they were for the scene that's going
		m_meshDrops.reset();
	}

	void SceneViewPanel::DrawMenuBar(EditorContext& context)
	{
		{
			const EditorUI::ScopedToolbarSpacing spacing;

			int& operation = m_gizmoState.m_currentOpMode;
			EditorUI::RadioOption("Translate", operation, static_cast<int>(ImGuizmo::TRANSLATE));
			EditorUI::ItemTooltip("Shortcut (W)");
			EditorUI::RadioOption("Rotate", operation, static_cast<int>(ImGuizmo::ROTATE));
			EditorUI::ItemTooltip("Shortcut (E)");
			EditorUI::RadioOption("Scale", operation, static_cast<int>(ImGuizmo::SCALE));
			EditorUI::ItemTooltip("Shortcut (R)");

			// Scaling is always along the object's own axes. Greyed out rather than hidden, so the centred toolbar
			// doesn't shift.
			int& space = m_gizmoState.m_transformationSpace;
			EditorUI::ToolbarGroupSeparator();
			ImGui::BeginDisabled(operation == ImGuizmo::SCALE);
			EditorUI::RadioOption("Local", space, static_cast<int>(ImGuizmo::LOCAL));
			EditorUI::ItemTooltip("Shortcut (Q)");
			EditorUI::RadioOption("World", space, static_cast<int>(ImGuizmo::WORLD));
			EditorUI::ItemTooltip("Shortcut (Q)");
			ImGui::EndDisabled();
		}

		m_gizmoState.m_snapper.DisplayControls(context.GetSettings(), m_gizmoState.m_currentOpMode);
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
			// the viewport size when it's created (SceneDocument::CreateEditorCamera).
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

			ImGui::Image(EditorUI::TextureID(texture), ImVec2(GetDimensions().X(), GetDimensions().Y()), ImVec2(0.0f, 0.0f), ImVec2(uvbx, uvby));
			const bool isImageHovered = ImGui::IsItemHovered();
			HandleAssetDrop(context, ImVec2(GetPosition().X(), GetPosition().Y() + cursorPos.y));

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
					const float* snap = snapper.PrepareManipulate(context.GetSettings(), m_gizmoState.m_currentOpMode, m_gizmoState.m_transformationSpace, ImGui::GetIO().KeyCtrl);

					ImGuizmo::Manipulate(view.GetValuePtr(), projection.GetValuePtr(), (ImGuizmo::OPERATION)m_gizmoState.m_currentOpMode, (ImGuizmo::MODE)m_gizmoState.m_transformationSpace, matrix, nullptr, snap);

					InteractionTracker& drag = m_gizmoState.m_drag;
					drag.Update(ImGuizmo::IsUsing());

					Vector3D newTranslation, newRotation, newScale;
					if (drag.IsActive())
					{
						ImGuizmo::DecomposeMatrixToComponents(matrix, newTranslation.GetValuePtr(), newRotation.GetValuePtr(), newScale.GetValuePtr());
					}

					snapper.Apply(drag, transformComponent->GetPosition(), transformComponent->GetRotation(), newTranslation, newRotation);

					// Applied once every panel has drawn; the gizmo picks the new transform up next frame
					if (drag.IsActive())
					{
						context.GetCommands().Push<SetTransformCommand>(selectedGameObject, newTranslation, newRotation, newScale, drag.GetID());

						const Vector3D& scale = transformComponent->GetScale();
						ApplyGizmoToOthers(context, transformComponent->GetPosition(), transformComponent->GetRotation(), scale,
							newTranslation, newRotation, newScale, drag.GetID());
					}
				}
				else
				{
					// Nothing to manipulate, so any drag in progress has ended
					m_gizmoState.m_drag.Update(false);
				}
			}

			// After the gizmo so it can't hide them; they're draw-only, so the gizmo still takes the clicks
			const ImVec2 viewOrigin(GetPosition().X(), GetPosition().Y() + cursorPos.y);
			DrawObjectIcons(context, viewOrigin);

			// After the icons so this frame's icon rects are picked against
			HandleSelectionInput(context, viewOrigin, isImageHovered);

			m_isImageShown = true;
		}
	}

	void SceneViewPanel::Update(EditorContext& context)
	{
		// Draw sets it again if the scene image still shows
		m_isImageShown = false;

		UpdateMeshDrops(context);
	}

	void SceneViewPanel::UpdateMeshDrops(EditorContext& context)
	{
		if (m_meshDrops == nullptr)
		{
			return;
		}

		// Their commands ran at the end of the last UI frame
		m_meshDrops->HandedOver.clear();

		std::vector<MeshDrops::Drop>& drops = m_meshDrops->Drops;
		for (auto drop = drops.begin(); drop != drops.end();)
		{
			if (drop->Mesh.IsLoading() || !drop->Position.has_value())
			{
				++drop;
				continue;
			}

			if (drop->Mesh.HasFailed())
			{
				DebugServices::Get().Trace(LogChannel::Info, "Couldn't load " + drop->Mesh.GetPath().GetRelativePath() + ", so nothing was added.");
			}
			else
			{
				// Loaded, so the command takes it from the cache rather than loading it on the main thread
				context.GetCommands().Push<CreateMeshObjectCommand>(drop->Mesh.GetPath(), *drop->Position);
				m_meshDrops->HandedOver.push_back(std::move(drop->Mesh));
			}
			drop = drops.erase(drop);
		}

		if (drops.empty() && m_meshDrops->HandedOver.empty())
		{
			m_meshDrops = nullptr;
		}
	}

	void SceneViewPanel::PreRender(EditorContext& context)
	{
		if (!m_isImageShown)
		{
			return;
		}

		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([this](GameObjectPtr gameObject)
			{
				GameObjectComponentPtr<CameraComponent> camera = gameObject->GetComponent<CameraComponent>();
				if (camera != nullptr)
				{
					DrawCameraFrustum(gameObject->GetTransformComponent()->GetPosition(), **camera);
				}
			}));

		// Last, so the grid's lines follow the opaque debug lines (frustums) and its depth writes can't hide them
		GameObjectPtr cameraObject = context.GetEditorCamera();
		if (cameraObject != nullptr)
		{
			m_grid.Draw(cameraObject->GetTransformComponent()->GetPosition(), context.GetSettings().GetSnapSize());
		}
	}

	void SceneViewPanel::ApplyGizmoToOthers(EditorContext& context, const Vector3D& fromPosition, const Vector3D& fromRotation, const Vector3D& fromScale,
		const Vector3D& toPosition, const Vector3D& toRotation, const Vector3D& toScale, InteractionID interaction)
	{
		const EditorSelection& selection = context.GetSelection();
		const GameObjectPtr primary = selection.GetPrimary();

		// This frame's change to the primary, applied to each of the others
		const Vector3D positionDelta = toPosition - fromPosition;
		const Quaternion fromOrientation = Quaternion::FromEulerDegrees(fromRotation);
		const Quaternion toOrientation = Quaternion::FromEulerDegrees(toRotation);
		const bool isWorldSpace = m_gizmoState.m_transformationSpace == ImGuizmo::WORLD;
		// World: rotate about the world axes the gizmo shows. Local: the same turn about each object's own axes.
		const Quaternion rotationDelta = isWorldSpace ? toOrientation * fromOrientation.Inverse() : fromOrientation.Inverse() * toOrientation;
		auto scaleRatio = [](float from, float to) { return from != 0.0f ? to / from : 1.0f; };
		const Vector3D scaleRatios(scaleRatio(fromScale.X(), toScale.X()), scaleRatio(fromScale.Y(), toScale.Y()), scaleRatio(fromScale.Z(), toScale.Z()));

		for (const GameObjectPtr& gameObject : selection.GetAll())
		{
			if (gameObject == primary || gameObject == nullptr)
			{
				continue;
			}

			// One whose parent is also selected moves with its parent already
			const bool hasSelectedAncestor = std::any_of(selection.GetAll().begin(), selection.GetAll().end(),
				[&gameObject](const GameObjectPtr& other)
				{
					return other != gameObject && other != nullptr && other->IsSelfOrAncestorOf(gameObject);
				});
			if (hasSelectedAncestor)
			{
				continue;
			}

			GameObjectComponentPtr<TransformComponent> transform = gameObject->GetTransformComponent();
			const Quaternion orientation = Quaternion::FromEulerDegrees(transform->GetRotation());
			const Quaternion newOrientation = isWorldSpace ? rotationDelta * orientation : orientation * rotationDelta;

			// Each turns about its own pivot, like the primary
			context.GetCommands().Push<SetTransformCommand>(gameObject,
				transform->GetPosition() + positionDelta,
				newOrientation.ToEulerDegrees(),
				transform->GetScale() * scaleRatios,
				interaction);
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
			m_selectStart = Vector2D(io.MousePos.x, io.MousePos.y);
		}

		if (!m_selectStart.has_value())
		{
			return;
		}

		// The marquee is kept to the scene image so it maps inside the camera's view
		const ImVec2 viewMax(viewOrigin.x + GetDimensions().X(), viewOrigin.y + GetDimensions().Y());
		const ImVec2 start(m_selectStart->X(), m_selectStart->Y());
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

		m_selectStart.reset();

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
		context.GetScenePicker().RequestPick(pickX, pickY, ScenePicker::ResultCallback([this, &selection, isAdditive](const ScenePick& pick)
			{
				ApplyClick(selection, pick.Object, isAdditive);
			}));
	}

	void SceneViewPanel::HandleAssetDrop(EditorContext& context, const ImVec2& viewOrigin)
	{
		if (!ImGui::BeginDragDropTarget())
		{
			return;
		}

		// Only meshes are taken, so anything else shows as not droppable
		const ImGuiPayload* const dragged = ImGui::GetDragDropPayload();
		const bool isMesh = dragged != nullptr && dragged->IsDataType(k_assetPathPayload)
			&& AssetPaths::IsMesh(Filepath(static_cast<const char*>(dragged->Data)));
		const ImGuiPayload* const dropped = isMesh ? ImGui::AcceptDragDropPayload(k_assetPathPayload) : nullptr;
		// Copied first: ending the target frees a payload that's been delivered
		const Filepath mesh = dropped != nullptr ? Filepath(static_cast<const char*>(dropped->Data)) : Filepath();
		ImGui::EndDragDropTarget();
		if (!mesh.IsValid())
		{
			return;
		}

		// The mesh loads in the background while the GPU pick finds where it goes; the object's added once both are in
		if (m_meshDrops == nullptr)
		{
			m_meshDrops = std::make_unique<MeshDrops>();
		}
		MeshDrops::Drop& drop = m_meshDrops->Drops.emplace_back();
		drop.ID = m_nextMeshDropID++;
		drop.Mesh.Load<StaticMeshResource>(mesh);

		const U64 dropID = drop.ID;
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const U32 pickX = static_cast<U32>(MathUtils::Clampf(mouse.x - viewOrigin.x, 0.0f, GetDimensions().X() - 1.0f));
		const U32 pickY = static_cast<U32>(MathUtils::Clampf(mouse.y - viewOrigin.y, 0.0f, GetDimensions().Y() - 1.0f));
		context.GetScenePicker().RequestPick(pickX, pickY, ScenePicker::ResultCallback([this, dropID](const ScenePick& pick)
			{
				// Gone if the scene changed since
				if (m_meshDrops == nullptr)
				{
					return;
				}
				for (MeshDrops::Drop& waiting : m_meshDrops->Drops)
				{
					if (waiting.ID == dropID)
					{
						waiting.Position = GetDropPosition(pick);
					}
				}
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

				const RenderObjectPtr& renderObject = renderComponent->GetRenderObject();
				if (renderObject == nullptr)
				{
					return;
				}

				// Each draw's box fits tighter than the whole object's box
				for (const MeshDraw& draw : renderObject->GetDraws())
				{
					if (frustum.Intersects(draw.Bounds))
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
