#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <UI/Gizmo/EditorGrid.h>
#include <UI/Gizmo/GizmoSnapper.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <vector>

struct ImVec2;
class CameraComponent;

namespace Rendering
{
	struct TexelReadback;
}

namespace Editor
{
	class EditorContext;
	class EditorSelection;

	class SceneViewPanel
	{
	private:
		struct TransformGizmoState
		{
			int m_currentOpMode;
			int m_transformationSpace;

			// Toggled with X; holding Ctrl during a drag inverts it
			GizmoSnapper m_snapper;
		};

		// Screen-space rect of a light or camera icon drawn this frame, so objects without a mesh can be picked
		struct IconHit
		{
			GameObjectPtr m_gameObject;
			Vector2D m_min;
			Vector2D m_max;
		};

	public:
		SceneViewPanel();
		~SceneViewPanel() = default;

	public:
		void Display(EditorContext& context);

		bool IsHovered();
		bool IsFocused();

		void Temp_RegisterInputs();

		// Icons at each light's and camera's position, plus camera frustums; viewOrigin is the screen-space top-left of the scene image
		void DrawObjectIcons(const EditorSelection& selection, const ImVec2& viewOrigin);
		// Debug lines along the camera's view volume, from its near plane out to its far plane or a fixed cap, whichever is closer
		void DrawCameraFrustum(const Vector3D& position, const CameraComponent& camera);

		const Vector2D& GetDimensions() const { return m_dimensions; }
		const Vector2D& GetPosition() const { return m_position; }

	private:
		// Left click selects, Ctrl+click toggles, and a drag selects everything in a marquee (Ctrl adds)
		void HandleSelectionInput(EditorSelection& selection, const ImVec2& viewOrigin, bool isImageHovered);
		// Every object with a mesh or icon inside the screen-space rect, including ones hidden behind others
		std::vector<GameObjectPtr> GatherObjectsInRect(const ImVec2& viewOrigin, const ImVec2& rectMin, const ImVec2& rectMax) const;
		// The topmost icon under the point, or null
		GameObjectPtr PickIcon(const ImVec2& point) const;
		// Selects (or with Ctrl, toggles) what a click hit; a miss without Ctrl clears the selection
		void ApplyClick(EditorSelection& selection, GameObjectPtr hitObject, bool isAdditive);

	private:
		bool m_isHovered = false;
		bool m_isFocused = false;
		Vector2D m_dimensions;
		Vector2D m_position;

		TransformGizmoState m_gizmoState;
		EditorGrid m_grid;

		std::vector<IconHit> m_iconHits;

		// Where the left button went down over the scene image; only meaningful while m_isSelecting
		Vector2D m_selectStart;
		bool m_isSelecting = false;

		// A click's GPU pick (PickRenderStage) while it's in flight; it takes a frame or two
		std::shared_ptr<Rendering::TexelReadback> m_pendingPick;
		bool m_isPendingPickAdditive = false;
	};

	inline bool SceneViewPanel::IsHovered()
	{
		return m_isHovered;
	}

	inline bool SceneViewPanel::IsFocused()
	{
		return m_isFocused;
	}
}