#pragma once

namespace Editor
{
	// Editor preferences that more than one panel or service reads. Owned by EditorContext; not saved between runs.
	class EditorSettings
	{
	public:
		// The snap size is always a whole number of these
		static constexpr float k_snapIncrement = 0.25f;
		static constexpr float k_maxSnapSize = 1.0f;

		// What the scene view's toolbar and the Settings window offer
		static constexpr float k_snapSizeOptions[] = { 0.25f, 0.5f, 0.75f, 1.0f };
		static constexpr float k_rotationStepOptions[] = { 15.0f, 30.0f, 45.0f, 90.0f };

	public:
		EditorSettings() = default;
		~EditorSettings() = default;

	public:
		// Metres. The gizmo's translation snap, the grid's square size, and how far each paste is offset.
		float GetSnapSize() const { return m_snapSize; }
		// Rounded to the nearest k_snapIncrement and clamped to [k_snapIncrement, k_maxSnapSize]
		void SetSnapSize(float snapSize);

		// Degrees. How far the gizmo turns per step while snapping.
		float GetRotationStep() const { return m_rotationStep; }
		void SetRotationStep(float rotationStep) { m_rotationStep = rotationStep; }

	private:
		float m_snapSize = 1.0f;
		float m_rotationStep = 15.0f;
	};
}
