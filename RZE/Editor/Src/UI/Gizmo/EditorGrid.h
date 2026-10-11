#pragma once

class Vector3D;

namespace Editor
{
	// The SceneView's ground grid on the Y = 0 plane, drawn as semi-transparent debug lines around the camera.
	// The base square size is the snap size, so snapped objects land on grid lines. As the camera rises,
	// squares grow tenfold, up to k_maxLevels times, so the grid stays readable when zoomed out; the smaller
	// squares fade out on the way rather than popping. Lines also fade with distance, so the grid has no
	// visible edge and new squares fade in as the camera approaches them.
	class EditorGrid
	{
	public:
		// Each level's squares are 10x the previous level's
		static constexpr int k_maxLevels = 2;

	public:
		EditorGrid() = default;
		~EditorGrid() = default;

	public:
		bool IsVisible() const { return m_isVisible; }

		// The size of the squares drawn last frame, in metres
		float GetSquareSize() const { return m_squareSize; }

		// The visibility toggle and current square size, for the SceneView menu bar
		void DisplayControls();

		// Queues this frame's grid lines. baseSquareSize is the snap size, in metres.
		void Draw(const Vector3D& cameraPosition, float baseSquareSize);

	private:
		bool m_isVisible = true;
		float m_squareSize = 1.0f;
	};
}
