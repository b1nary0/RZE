#pragma once

#include <Utils/Math/Vector2D.h>

namespace Editor
{

	class SceneViewPanel
	{
	public:
		SceneViewPanel() = default;
		~SceneViewPanel() = default;

	public:
		void Display();

		bool IsHovered();
		bool IsFocused();

	private:
		bool bIsHovered = false;
		bool bIsFocused = false;
		Vector2D mDimensions;
	};

	inline bool SceneViewPanel::IsHovered()
	{
		return bIsHovered;
	}

	inline bool SceneViewPanel::IsFocused()
	{
		return bIsFocused;
	}
}