#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <UI/Framework/EditorPanel.h>
#include <UI/Framework/InteractionTracker.h>
#include <UI/Gizmo/EditorGrid.h>
#include <UI/Gizmo/GizmoSnapper.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <vector>

struct ImVec2;
class CameraComponent;

namespace Editor
{
	class EditorSelection;

	// The scene rendered from the editor camera, with the transform gizmo, light and camera icons, the grid, and
	// click/marquee selection. Its menu bar holds the gizmo, snapping and grid controls.
	class SceneViewPanel : public EditorPanel
	{
	private:
		struct TransformGizmoState
		{
			int m_currentOpMode;
			int m_transformationSpace;

			// Toggled with X; holding Ctrl during a drag inverts it
			GizmoSnapper m_snapper;

			// Follows ImGuizmo::IsUsing(), so each drag's commands share an interaction
			InteractionTracker m_drag;
		};

		// Screen-space rect of a light or camera icon drawn this frame, so objects without a mesh can be picked
		struct IconHit
		{
			GameObjectPtr m_gameObject;
			Vector2D m_min;
			Vector2D m_max;
		};

	public:
		static PanelDesc Describe();

		SceneViewPanel();

	private:
		void Initialize(EditorContext& context) override;
		void DrawMenuBar(EditorContext& context) override;
		void Draw(EditorContext& context) override;
		void OnSceneReset() override;
		void PushWindowStyle(PanelWindowStyle& style) const override;

		// W/E/R/Q/X gizmo shortcuts. They only reach the panel while the SceneView is hovered (EditorApp::ProcessInput).
		void RegisterInputs();

		const Vector2D& GetDimensions() const { return m_dimensions; }
		const Vector2D& GetPosition() const { return m_position; }

		// Applies this frame's gizmo change to the primary (from -> to) to every other selected object, as one
		// interaction with the primary's. Positions move by the same offset, rotations turn by the same amount about
		// each object's own pivot, and scales scale by the same ratio.
		void ApplyGizmoToOthers(EditorContext& context, const Vector3D& fromPosition, const Vector3D& fromRotation, const Vector3D& fromScale,
			const Vector3D& toPosition, const Vector3D& toRotation, const Vector3D& toScale, InteractionID interaction);

		// Icons at each light's and camera's position, plus camera frustums; viewOrigin is the screen-space top-left of the scene image
		void DrawObjectIcons(const EditorContext& context, const ImVec2& viewOrigin);
		// Debug lines along the camera's view volume, from its near plane out to its far plane or a fixed cap, whichever is closer
		void DrawCameraFrustum(const Vector3D& position, const CameraComponent& camera);

		// Left click selects, Ctrl+click toggles, and a drag selects everything in a marquee (Ctrl adds)
		void HandleSelectionInput(EditorContext& context, const ImVec2& viewOrigin, bool isImageHovered);
		// Every object with a mesh or icon inside the screen-space rect, including ones hidden behind others
		std::vector<GameObjectPtr> GatherObjectsInRect(const EditorContext& context, const ImVec2& viewOrigin, const ImVec2& rectMin, const ImVec2& rectMax) const;
		// The topmost icon under the point, or null
		GameObjectPtr PickIcon(const ImVec2& point) const;
		// Selects (or with Ctrl, toggles) what a click hit; a miss without Ctrl clears the selection
		void ApplyClick(EditorSelection& selection, GameObjectPtr hitObject, bool isAdditive);

	private:
		Vector2D m_dimensions;
		Vector2D m_position;

		TransformGizmoState m_gizmoState;
		EditorGrid m_grid;

		std::vector<IconHit> m_iconHits;

		// Where the left button went down over the scene image; only meaningful while m_isSelecting
		Vector2D m_selectStart;
		bool m_isSelecting = false;
	};
}