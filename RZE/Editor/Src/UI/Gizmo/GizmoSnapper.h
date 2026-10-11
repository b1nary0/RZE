#pragma once

#include <Utils/Math/Vector3D.h>

#include <GLM/gtc/quaternion.hpp>

namespace Editor
{
	// Snapping for the SceneView transform gizmo: step-wise rotation and snap-to-grid translation.
	// Owns the snap settings and the state of the drag in progress. Per frame, call PrepareManipulate
	// before ImGuizmo::Manipulate and pass its result as the snap argument, then Apply to the
	// translation and rotation ImGuizmo produced.
	class GizmoSnapper
	{
	public:
		// The snap size is always a whole number of these
		static constexpr float k_snapIncrement = 0.25f;
		static constexpr float k_maxSnapSize = 1.0f;

	private:
		struct DragState
		{
			bool m_wasUsing = false;
			// Which world axes the current translate drag has moved; only those get snapped, so axis and plane drags stay on their axes
			Vector3D m_startPosition;
			bool m_hasAxisMoved[3] = { false, false, false };
			// The rotation at the start of the drag, and where the drag would have taken it without snapping
			glm::quat m_startRotation;
			glm::quat m_unsnappedRotation;
		};

	public:
		GizmoSnapper() = default;
		~GizmoSnapper() = default;

	public:
		bool IsEnabled() const { return m_isEnabled; }
		void ToggleEnabled() { m_isEnabled = !m_isEnabled; }

		// Metres; rounded to the nearest k_snapIncrement and clamped to [k_snapIncrement, k_maxSnapSize]
		float GetSnapSize() const { return m_snapSize; }
		void SetSnapSize(float snapSize);

		// Degrees
		float GetRotationStep() const { return m_rotationStep; }
		void SetRotationStep(float rotationStep) { m_rotationStep = rotationStep; }

		// The snap toggle and radio buttons for the snap size and rotation step, for the SceneView menu bar
		void DisplayControls();

		// Decides whether this frame snaps (the toggle, inverted while isInverted) and returns the snap
		// array to hand ImGuizmo::Manipulate, or null when ImGuizmo shouldn't snap.
		// operation and mode are ImGuizmo::OPERATION and ImGuizmo::MODE.
		const float* PrepareManipulate(int operation, int mode, bool isInverted);

		// After ImGuizmo::Manipulate, every frame so it sees drags start and end. currentPosition and currentRotation
		// (Euler degrees) are the object's before this frame's manipulation; translation and rotation are what
		// ImGuizmo produced, and are snapped in place.
		void Apply(bool isUsing, const Vector3D& currentPosition, const Vector3D& currentRotation, Vector3D& translation, Vector3D& rotation);

	private:
		void SnapTranslationToGrid(Vector3D& translation);
		void SnapRotationToSteps(const Vector3D& currentRotation, Vector3D& rotation);

	private:
		bool m_isEnabled = false;
		float m_snapSize = 1.0f;
		float m_rotationStep = 15.0f;

		// Set by PrepareManipulate for this frame
		bool m_isGridSnapping = false;
		bool m_isRotationSnapping = false;
		float m_imguizmoSnap[3] = { 0.0f, 0.0f, 0.0f };

		DragState m_drag;
	};
}
