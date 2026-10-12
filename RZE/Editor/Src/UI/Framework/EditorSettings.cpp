#include <UI/Framework/EditorSettings.h>

#include <Utils/Math/Math.h>

#include <cmath>

namespace Editor
{
	void EditorSettings::SetSnapSize(float snapSize)
	{
		const float increments = std::round(snapSize / k_snapIncrement);
		m_snapSize = MathUtils::Clampf(increments * k_snapIncrement, k_snapIncrement, k_maxSnapSize);
	}
}
