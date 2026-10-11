#include <UI/Gizmo/GizmoSnapper.h>

#include <UI/ToolbarLayout.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui.h>
#include <imGUI/ImGuizmo.h>

#include <cmath>
#include <stdio.h>

namespace
{
	constexpr float k_snapSizes[] = { 0.25f, 0.5f, 0.75f, 1.0f };
	constexpr float k_rotationSteps[] = { 15.0f, 30.0f, 45.0f, 90.0f };
	// An axis counts as moved once it's this far from where the drag started
	constexpr float k_movedEpsilon = 1e-4f;

	// Euler order matches TransformComponent and ImGuizmo (see Units.h)
	glm::quat EulerDegreesToQuat(const Vector3D& eulerDegrees)
	{
		return glm::quat(eulerDegrees.GetInternalVec() * MathUtils::ToRadians);
	}
}

namespace Editor
{
	void GizmoSnapper::SetSnapSize(float snapSize)
	{
		const float increments = std::round(snapSize / k_snapIncrement);
		m_snapSize = MathUtils::Clampf(increments * k_snapIncrement, k_snapIncrement, k_maxSnapSize);
	}

	void GizmoSnapper::DisplayControls()
	{
		ToolbarLayout::PushGroupSpacing();

		ToolbarLayout::BeginGroup();
		ImGui::Checkbox("Snap (X)", &m_isEnabled);

		ToolbarLayout::BeginGroup();
		ImGui::TextUnformatted("Move step:");
		for (float snapSize : k_snapSizes)
		{
			char label[32];
			snprintf(label, sizeof(label), "%g m##SnapSize", snapSize);
			if (ImGui::RadioButton(label, m_snapSize == snapSize))
			{
				SetSnapSize(snapSize);
			}
		}

		ToolbarLayout::BeginGroup();
		ImGui::TextUnformatted("Rotate step:");
		for (float rotationStep : k_rotationSteps)
		{
			char label[32];
			// UTF-8 degree sign, spelled out so it doesn't depend on the source file's encoding
			snprintf(label, sizeof(label), "%g\xC2\xB0##RotationStep", rotationStep);
			if (ImGui::RadioButton(label, m_rotationStep == rotationStep))
			{
				m_rotationStep = rotationStep;
			}
		}

		ImGui::PopStyleVar();
	}

	const float* GizmoSnapper::PrepareManipulate(int operation, int mode, bool isInverted)
	{
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

	void GizmoSnapper::Apply(bool isUsing, const Vector3D& currentPosition, const Vector3D& currentRotation, Vector3D& translation, Vector3D& rotation)
	{
		if (isUsing && !m_drag.m_wasUsing)
		{
			m_drag.m_startPosition = currentPosition;
			m_drag.m_hasAxisMoved[0] = m_drag.m_hasAxisMoved[1] = m_drag.m_hasAxisMoved[2] = false;
			m_drag.m_startRotation = EulerDegreesToQuat(currentRotation);
			m_drag.m_unsnappedRotation = m_drag.m_startRotation;
		}
		m_drag.m_wasUsing = isUsing;

		if (!isUsing)
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
		glm::vec3& translationVec = const_cast<glm::vec3&>(translation.GetInternalVec());
		const glm::vec3& startVec = m_drag.m_startPosition.GetInternalVec();
		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(translationVec[axis] - startVec[axis]) > k_movedEpsilon)
			{
				m_drag.m_hasAxisMoved[axis] = true;
			}

			if (m_drag.m_hasAxisMoved[axis])
			{
				translationVec[axis] = std::round(translationVec[axis] / m_snapSize) * m_snapSize;
			}
		}
	}

	void GizmoSnapper::SnapRotationToSteps(const Vector3D& currentRotation, Vector3D& rotation)
	{
		// ImGuizmo rotates by the mouse's movement since last frame, applied to the (snapped) current rotation.
		// Snapping that directly would swallow slow drags, so the movement is accumulated unsnapped and the
		// total turn since the drag started is what gets snapped.
		const glm::quat frameDelta = EulerDegreesToQuat(rotation) * glm::inverse(EulerDegreesToQuat(currentRotation));
		m_drag.m_unsnappedRotation = glm::normalize(frameDelta * m_drag.m_unsnappedRotation);

		const glm::quat totalDelta = m_drag.m_unsnappedRotation * glm::inverse(m_drag.m_startRotation);
		const float step = m_rotationStep * MathUtils::ToRadians;
		const float snappedAngle = std::round(glm::angle(totalDelta) / step) * step;
		const glm::quat snapped = glm::angleAxis(snappedAngle, glm::axis(totalDelta)) * m_drag.m_startRotation;

		const glm::vec3 eulerRadians = glm::eulerAngles(snapped);
		rotation = Vector3D(eulerRadians.x, eulerRadians.y, eulerRadians.z) * MathUtils::ToDegrees;
	}
}
