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

	public:
		EditorSettings() = default;
		~EditorSettings() = default;

	public:
		// Metres. The gizmo's translation snap, the grid's square size, and how far each paste is offset.
		float GetSnapSize() const { return m_snapSize; }
		// Rounded to the nearest k_snapIncrement and clamped to [k_snapIncrement, k_maxSnapSize]
		void SetSnapSize(float snapSize);

	private:
		float m_snapSize = 1.0f;
	};
}
