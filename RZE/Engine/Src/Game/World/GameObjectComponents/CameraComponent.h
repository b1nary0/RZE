#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/RenderView.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class TransformComponent;

class CameraComponent final : public GameObjectComponent<CameraComponent>
{
public:
	CameraComponent();
	~CameraComponent() = default;

	// GameObjectComponent interface
public:
	void Initialize() override;
	void OnAddToScene() override;
	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	void OnEditorInspect() override;

public:
	// Derived from the owner's TransformComponent rotation; an unrotated camera looks down -Z with +Y up
	Vector3D GetForward() const;
	Vector3D GetUpDir() const;

	const Matrix4x4& GetProjectionMatrix() const;
	const Matrix4x4& GetViewMatrix() const;

	float GetFOV() const;
	float GetAspectRatio() const;
	float GetNearCull() const;
	float GetFarCull() const;
	float GetExposureCompensation() const;

	bool IsActiveCamera() const;

	void SetFOV(float fov);
	void SetAspectRatio(float aspectRatio);
	void SetNearCull(float nearCull);
	void SetFarCull(float farCull);
	void SetExposureCompensation(float ev);

	void SetAsActiveCamera(bool isActiveCamera);

private:
	void GenerateCameraMatrices(const Vector3D& position);

private:
	Matrix4x4 m_projectionMat;
	Matrix4x4 m_viewMat;

	float m_fov { 60.0f };
	float m_aspectRatio { 0.0f }; // #TODO I don't think we're updating this sensibly when the aspect ratio changes...
	float m_nearCull { 0.01f };
	float m_farCull { 1000.0f };
	// EV on top of auto-exposure; lets a scene sit darker (low-key) or brighter (high-key) than the default
	float m_exposureCompensation { 0.0f };

	bool m_isActiveCamera { false };

	// #TODO this should only be a thing in editor
	std::unique_ptr<Rendering::RenderTargetTexture> m_renderTarget;
	// The camera preview in the inspector; renders into m_renderTarget and keeps its own per-view render state
	std::unique_ptr<RenderView> m_previewView;

};