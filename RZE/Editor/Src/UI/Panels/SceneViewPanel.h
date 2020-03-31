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

	private:
		Vector2D mDimensions;
	};

}