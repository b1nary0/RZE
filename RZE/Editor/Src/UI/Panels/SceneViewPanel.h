#pragma once

#include <Utils/Math/Vector2D.h>

namespace Editor
{
	class SceneViewPanel
	{
	private:
		struct TransformGizmoState
		{
			int m_currentOpMode;
			int m_transformationSpace;
		};

	public:
		SceneViewPanel();
		~SceneViewPanel() = default;

	public:
		void Display();

		bool IsHovered();
		bool IsFocused();

		void Temp_RegisterInputs();

		const Vector2D& GetDimensions() const { return m_dimensions; }
		const Vector2D& GetPosition() const { return m_position; }

	private:
		bool m_isHovered = false;
		bool m_isFocused = false;
		Vector2D m_dimensions;
		Vector2D m_position;

		TransformGizmoState m_gizmoState;
	};

	inline bool SceneViewPanel::IsHovered()
	{
		return m_isHovered;
	}

	inline bool SceneViewPanel::IsFocused()
	{
		return m_isFocused;
	}
}