#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class TransformComponent;

class CameraComponent final : public GameObjectComponent<CameraComponent>
{
public:
	CameraComponent() = default;
	~CameraComponent() = default;

	// GameObjectComponent interface
public:
	void OnAddToScene() override;
	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	void OnEditorInspect() override;

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

private:
	Vector3D m_lookAt;
	Vector3D m_upDir { 0.0f, 1.0f, 0.0f };
	Vector3D m_forward;

	Matrix4x4 m_projectionMat;
	Matrix4x4 m_viewMat;

	float m_fov { 60.0f };
	float m_aspectRatio { 0.0f }; // #TODO I don't think we're updating this sensibly when the aspect ratio changes...
	float m_nearCull { 0.01f };
	float m_farCull { 1000.0f };

	bool m_isActiveCamera { false };

	// #TODO this should only be a thing in editor
	std::unique_ptr<Rendering::RenderTargetTexture> m_renderTarget;
};