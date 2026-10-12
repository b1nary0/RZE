#include <UI/Gizmo/EditorGrid.h>

#include <UI/Framework/EditorUI.h>

#include <RZE.h>
#include <Graphics/RenderEngine.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Vector3D.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <cmath>
#include <stdio.h>

namespace
{
	// Every this many lines is a major line
	constexpr int k_majorLineInterval = 10;
	// Squares start shrinking out of view once the camera is this many of them above the grid, and the
	// next level (10x squares) has fully taken over at 10x that height
	constexpr float k_levelStartHeightInSquares = 5.0f;

	// Lines fade out with distance from the point under the camera. The fade ends at this multiple of
	// the camera's height (so it widens smoothly as the camera rises, with no jump between levels),
	// kept within [k_minFadeEndInSquares of the snap size, k_maxFadeEndInSquares of the current squares].
	constexpr float k_fadeEndHeightRatio = 4.0f;
	constexpr float k_minFadeEndInSquares = 40.0f;
	// At least k_fadeEndHeightRatio * 10 * k_levelStartHeightInSquares, the fade end just before a level steps up,
	// or the fade would jump when it does
	constexpr float k_maxFadeEndInSquares = 200.0f;
	// Lines are fully visible out to this fraction of the fade end
	constexpr float k_fadeStartFraction = 0.3f;

	const Vector3D k_lineColour(0.75f, 0.75f, 0.75f);
	constexpr float k_minorAlpha = 0.25f;
	constexpr float k_majorAlpha = 0.5f;
	// The lines through the origin take their axis colours: X red, Z blue
	const Vector3D k_xAxisColour(0.9f, 0.3f, 0.3f);
	const Vector3D k_zAxisColour(0.3f, 0.45f, 0.9f);
	constexpr float k_axisAlpha = 0.7f;

	// How many squares of this level the line at index (in squares from the origin) is the boundary of:
	// 0 for a minor line, 1 for a major line, 2 for a major line of the next level up
	int GetLineTier(int index)
	{
		if (index % (k_majorLineInterval * k_majorLineInterval) == 0)
		{
			return 2;
		}
		return index % k_majorLineInterval == 0 ? 1 : 0;
	}

	// levelProgress (0..1) is how far the camera is through this level towards the next. Minor lines fade out
	// over it while major lines dim to minor, so when the level steps up, the old major lines carry on as the
	// new minor lines at the same alpha and nothing pops.
	float GetLineAlpha(int tier, float levelProgress)
	{
		const float relativeTier = tier - levelProgress;
		if (relativeTier < 0.0f)
		{
			return k_minorAlpha * (1.0f + relativeTier);
		}
		return k_minorAlpha + (k_majorAlpha - k_minorAlpha) * std::min(relativeTier, 1.0f);
	}
}

namespace Editor
{
	void EditorGrid::DisplayControls()
	{
		const EditorUI::ScopedToolbarSpacing spacing;

		EditorUI::ToolbarGroupSeparator();
		ImGui::Checkbox("Grid", &m_isVisible);
		if (m_isVisible)
		{
			// So it's clear which level is showing once the squares outgrow the snap size
			ImGui::TextDisabled("%g m squares", m_squareSize);
		}
	}

	void EditorGrid::Draw(const Vector3D& cameraPosition, float baseSquareSize)
	{
		// Levels as a continuous value, so the fades follow the camera smoothly
		const float height = std::abs(cameraPosition.Y());
		const float levelStartHeight = k_levelStartHeightInSquares * baseSquareSize;
		const float continuousLevel = MathUtils::Clampf(std::log10(std::max(height, 1e-3f) / levelStartHeight), 0.0f, static_cast<float>(k_maxLevels));
		const int level = std::min(static_cast<int>(continuousLevel), k_maxLevels);
		const float levelProgress = continuousLevel - level;

		const float squareSize = baseSquareSize * std::pow(10.0f, static_cast<float>(level));
		m_squareSize = squareSize;

		if (!m_isVisible)
		{
			return;
		}

		DebugLineFade fade;
		fade.centre = Vector3D(cameraPosition.X(), 0.0f, cameraPosition.Z());
		fade.end = MathUtils::Clampf(k_fadeEndHeightRatio * height, k_minFadeEndInSquares * baseSquareSize, k_maxFadeEndInSquares * squareSize);
		fade.start = k_fadeStartFraction * fade.end;

		// Only out to where the lines have faded, centred on the camera in whole major cells so the lines don't
		// slide as it moves (hence the extra major cell). New lines appear past the fade and fade in as the camera
		// approaches. Counted in squares from the origin so major and axis lines are found exactly.
		const int halfExtentCells = static_cast<int>(std::ceil(fade.end / squareSize)) + k_majorLineInterval;
		const float majorCellSize = squareSize * k_majorLineInterval;
		const int centreX = static_cast<int>(std::round(cameraPosition.X() / majorCellSize)) * k_majorLineInterval;
		const int centreZ = static_cast<int>(std::round(cameraPosition.Z() / majorCellSize)) * k_majorLineInterval;

		const float minX = (centreX - halfExtentCells) * squareSize;
		const float maxX = (centreX + halfExtentCells) * squareSize;
		const float minZ = (centreZ - halfExtentCells) * squareSize;
		const float maxZ = (centreZ + halfExtentCells) * squareSize;

		RenderEngine& renderEngine = RZE().GetRenderEngine();

		auto drawLine = [&](const Vector3D& start, const Vector3D& end, int index, const Vector3D& axisColour)
		{
			if (index == 0)
			{
				renderEngine.DrawLine(start, end, axisColour, k_axisAlpha, fade);
				return;
			}

			const float alpha = GetLineAlpha(GetLineTier(index), levelProgress);
			if (alpha > 0.0f)
			{
				renderEngine.DrawLine(start, end, k_lineColour, alpha, fade);
			}
		};

		for (int offset = -halfExtentCells; offset <= halfExtentCells; ++offset)
		{
			// Lines of constant X run along Z; the one at X = 0 is the Z axis
			const int xIndex = centreX + offset;
			const float x = xIndex * squareSize;
			drawLine(Vector3D(x, 0.0f, minZ), Vector3D(x, 0.0f, maxZ), xIndex, k_zAxisColour);

			// Lines of constant Z run along X; the one at Z = 0 is the X axis
			const int zIndex = centreZ + offset;
			const float z = zIndex * squareSize;
			drawLine(Vector3D(minX, 0.0f, z), Vector3D(maxX, 0.0f, z), zIndex, k_xAxisColour);
		}
	}
}
