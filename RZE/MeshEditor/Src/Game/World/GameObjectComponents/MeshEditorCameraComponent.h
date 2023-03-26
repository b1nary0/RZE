#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class TransformComponent;

namespace MeshEditor
{
	class MeshEditorCameraComponent final : public GameObjectComponent<MeshEditorCameraComponent>
	{
		static constexpr float kMinDirectionHeldTime = 1.f;
		static constexpr float kMaxDirectionHeldTime = 10.f;
	public:
		MeshEditorCameraComponent() = default;
		~MeshEditorCameraComponent() = default;

		// GameObjectComponent interface
		public:
		void Initialize() override;
		void OnAddToScene() override;
		void Update() override;

	public:
		const Vector3D& GetLookAt() const;
		const Vector3D& GetUpDir() const;
		const Vector3D& GetForward() const;

		const Matrix4x4& GetProjectionMatrix() const;
		const Matrix4x4& GetViewMatrix() const;

		float GetFOV() const;
		float GetAspectRatio() const;
		float GetNearCull() const;
		float GetFarCull() const;

		bool IsActiveCamera() const;

		void SetLookAt(const Vector3D& lookAt);
		void SetUpDir(const Vector3D& upDir);
		void SetForward(const Vector3D& forward);

		void SetFOV(float fov);
		void SetAspectRatio(float aspectRatio);
		void SetNearCull(float nearCull);
		void SetFarCull(float farCull);

		void SetAsActiveCamera(bool isActiveCamera);

	private:
		void GenerateCameraMatrices(const Vector3D& position);

		void KeyboardInput(GameObjectComponentPtr<TransformComponent>& transfComp);
		void MouseInput(GameObjectComponentPtr<TransformComponent>& transfComp);

		void SetSpeedRampMultiplier(bool isMoving, float growingDelta);

	private:
		Vector3D m_lookAt;
		Vector3D m_upDir{ 0.0f, 1.0f, 0.0f };
		Vector3D m_forward{ 0.0f, 0.0f, -1.0f };

		Matrix4x4 m_projectionMat;
		Matrix4x4 m_viewMat;

		float m_fov{ 70.0f };
		float m_aspectRatio{ 0.0f }; // #TODO I don't think we're updating this sensibly when the aspect ratio changes...
		float m_nearCull{ 0.01f };
		float m_farCull{ 1000.0f };

		float m_speed;
		float m_deltaSpeedRampMultiplier = kMinDirectionHeldTime;
		float m_wheelZoomSpeed;

		Vector3D m_mousePrevPos;
		Vector3D m_yawPitchRoll;

		bool m_isActiveCamera{ false };
	};
}