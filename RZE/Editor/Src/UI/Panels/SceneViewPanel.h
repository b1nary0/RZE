#pragma once

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

struct ImVec2;
class CameraComponent;

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

		// Icons at each light's and camera's position, plus camera frustums; viewOrigin is the screen-space top-left of the scene image
		void DrawObjectIcons(const ImVec2& viewOrigin);
		// Debug lines along the camera's view volume, from its near plane out to its far plane or a fixed cap, whichever is closer
		void DrawCameraFrustum(const Vector3D& position, const CameraComponent& camera);

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