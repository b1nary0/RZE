#pragma once

#include <UI/Framework/InteractionTracker.h>

#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>

namespace Editor
{
	class EditorSettings;

	// Snapping for the SceneView transform gizmo: step-wise rotation and snap-to-grid translation.
	// Owns the snap toggle and the state of the drag in progress; the snap size and rotation step are shared
	// (EditorSettings). Per frame, call PrepareManipulate
	// before ImGuizmo::Manipulate and pass its result as the snap argument, then Apply to the
	// translation and rotation ImGuizmo produced.
	class GizmoSnapper
	{
	private:
		struct DragState
		{
			// Which world axes the current translate drag has moved; only those get snapped, so axis and plane drags stay on their axes
			Vector3D m_startPosition;
			bool m_hasAxisMoved[3] = { false, false, false };
			// The rotation at the start of the drag, and where the drag would have taken it without snapping
			Quaternion m_startRotation;
			Quaternion m_unsnappedRotation;
		};

	public:
		GizmoSnapper() = default;
		~GizmoSnapper() = default;

	public:
		bool IsEnabled() const { return m_isEnabled; }
		void ToggleEnabled() { m_isEnabled = !m_isEnabled; }

		// The snap toggle and radio buttons for the snap size and rotation step, for the SceneView menu bar. Those that
		// operation (ImGuizmo::OPERATION) doesn't use are greyed out.
		void DisplayControls(EditorSettings& settings, int operation);

		// One radio button per EditorSettings option, on the line of what's before them (e.g. a label)
		static void DrawSnapSizeOptions(EditorSettings& settings);
		static void DrawRotationStepOptions(EditorSettings& settings);

		// Decides whether this frame snaps (the toggle, inverted while isInverted) and returns the snap
		// array to hand ImGuizmo::Manipulate, or null when ImGuizmo shouldn't snap.
		// operation and mode are ImGuizmo::OPERATION and ImGuizmo::MODE.
		const float* PrepareManipulate(const EditorSettings& settings, int operation, int mode, bool isInverted);

		// After ImGuizmo::Manipulate. gizmoDrag tracks ImGuizmo::IsUsing(), so a drag's start is where snapping is
		// measured from. currentPosition and currentRotation (Euler degrees) are the object's before this frame's
		// manipulation; translation and rotation are what ImGuizmo produced, and are snapped in place.
		void Apply(const InteractionTracker& gizmoDrag, const Vector3D& currentPosition, const Vector3D& currentRotation, Vector3D& translation, Vector3D& rotation);

	private:
		void SnapTranslationToGrid(Vector3D& translation);
		void SnapRotationToSteps(const Vector3D& currentRotation, Vector3D& rotation);

	private:
		bool m_isEnabled = false;

		// Set by PrepareManipulate for this frame
		float m_snapSize = 1.0f;
		float m_rotationStep = 15.0f;
		bool m_isGridSnapping = false;
		bool m_isRotationSnapping = false;
		float m_imguizmoSnap[3] = { 0.0f, 0.0f, 0.0f };

		DragState m_drag;
	};
}
