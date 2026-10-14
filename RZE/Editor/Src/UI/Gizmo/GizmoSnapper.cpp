#include <UI/Gizmo/GizmoSnapper.h>

#include <UI/Framework/EditorSettings.h>
#include <UI/Framework/EditorUI.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

#include <cmath>
#include <stdio.h>

namespace
{
	// An axis counts as moved once it's this far from where the drag started
	constexpr float k_movedEpsilon = 1e-4f;
}

namespace Editor
{
	void GizmoSnapper::DisplayControls(EditorSettings& settings)
	{
		const EditorUI::ScopedToolbarSpacing spacing;

		EditorUI::ToolbarGroupSeparator();
		ImGui::Checkbox("Snap (X)", &m_isEnabled);

		EditorUI::ToolbarGroupSeparator();
		ImGui::TextUnformatted("Move step:");
		DrawSnapSizeOptions(settings);

		EditorUI::ToolbarGroupSeparator();
		ImGui::TextUnformatted("Rotate step:");
		DrawRotationStepOptions(settings);
	}

	void GizmoSnapper::DrawSnapSizeOptions(EditorSettings& settings)
	{
		for (float snapSize : EditorSettings::k_snapSizeOptions)
		{
			char label[32];
			snprintf(label, sizeof(label), "%g m##SnapSize", snapSize);
			ImGui::SameLine();
			if (ImGui::RadioButton(label, settings.GetSnapSize() == snapSize))
			{
				settings.SetSnapSize(snapSize);
			}
		}
	}

	void GizmoSnapper::DrawRotationStepOptions(EditorSettings& settings)
	{
		for (float rotationStep : EditorSettings::k_rotationStepOptions)
		{
			char label[32];
			// UTF-8 degree sign, spelled out so it doesn't depend on the source file's encoding
			snprintf(label, sizeof(label), "%g\xC2\xB0##RotationStep", rotationStep);
			ImGui::SameLine();
			if (ImGui::RadioButton(label, settings.GetRotationStep() == rotationStep))
			{
				settings.SetRotationStep(rotationStep);
			}
		}
	}

	const float* GizmoSnapper::PrepareManipulate(const EditorSettings& settings, int operation, int mode, bool isInverted)
	{
		m_snapSize = settings.GetSnapSize();
		m_rotationStep = settings.GetRotationStep();

		const bool isSnapping = m_isEnabled != isInverted;
		const bool isTranslating = operation == ImGuizmo::TRANSLATE;

		// ImGuizmo snaps translation in steps from the drag start, not to the grid, so in world space
		// Apply snaps to the grid instead. Local axes don't line up with the grid, so there ImGuizmo's
		// relative steps are the best fit.
		m_isGridSnapping = isSnapping && isTranslating && mode == ImGuizmo::WORLD;
		// ImGuizmo snaps the absolute angle on the rotation plane rather than the angle turned since the drag
		// started, so its steps are off by however far the click was from a step; Apply snaps rotation instead
		m_isRotationSnapping = isSnapping && operation == ImGuizmo::ROTATE;

		if (isSnapping && isTranslating && !m_isGridSnapping)
		{
			m_imguizmoSnap[0] = m_imguizmoSnap[1] = m_imguizmoSnap[2] = m_snapSize;
			return m_imguizmoSnap;
		}

		return nullptr;
	}

	void GizmoSnapper::Apply(const InteractionTracker& gizmoDrag, const Vector3D& currentPosition, const Vector3D& currentRotation, Vector3D& translation, Vector3D& rotation)
	{
		if (gizmoDrag.HasBegun())
		{
			m_drag.m_startPosition = currentPosition;
			m_drag.m_hasAxisMoved[0] = m_drag.m_hasAxisMoved[1] = m_drag.m_hasAxisMoved[2] = false;
			// Euler order matches TransformComponent and ImGuizmo (see Units.h)
			m_drag.m_startRotation = Quaternion::FromEulerDegrees(currentRotation);
			m_drag.m_unsnappedRotation = m_drag.m_startRotation;
		}
		if (!gizmoDrag.IsActive())
		{
			return;
		}

		if (m_isGridSnapping)
		{
			SnapTranslationToGrid(translation);
		}

		if (m_isRotationSnapping)
		{
			SnapRotationToSteps(currentRotation, rotation);
		}
	}

	void GizmoSnapper::SnapTranslationToGrid(Vector3D& translation)
	{
		// No drift: ImGuizmo takes each frame's delta from the mouse to the current matrix, so rounding here doesn't accumulate
		float* translationValues = translation.GetValuePtr();
		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(translationValues[axis] - m_drag.m_startPosition[axis]) > k_movedEpsilon)
			{
				m_drag.m_hasAxisMoved[axis] = true;
			}

			if (m_drag.m_hasAxisMoved[axis])
			{
				translationValues[axis] = std::round(translationValues[axis] / m_snapSize) * m_snapSize;
			}
		}
	}

	void GizmoSnapper::SnapRotationToSteps(const Vector3D& currentRotation, Vector3D& rotation)
	{
		// ImGuizmo rotates by the mouse's movement since last frame, applied to the (snapped) current rotation.
		// Snapping that directly would swallow slow drags, so the movement is accumulated unsnapped and the
		// total turn since the drag started is what gets snapped.
		const Quaternion frameDelta = Quaternion::FromEulerDegrees(rotation) * Quaternion::FromEulerDegrees(currentRotation).Inverse();
		m_drag.m_unsnappedRotation = (frameDelta * m_drag.m_unsnappedRotation).Normalized();

		const Quaternion totalDelta = m_drag.m_unsnappedRotation * m_drag.m_startRotation.Inverse();
		const float step = m_rotationStep * MathUtils::ToRadians;
		const float snappedAngle = std::round(totalDelta.ToAngleRadians() / step) * step;
		const Quaternion snapped = Quaternion::FromAngleAxis(snappedAngle, totalDelta.ToAxis()) * m_drag.m_startRotation;

		rotation = snapped.ToEulerDegrees();
	}
}
