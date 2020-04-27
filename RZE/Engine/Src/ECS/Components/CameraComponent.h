#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

struct CameraComponent final : public Apollo::Component<CameraComponent>
{
	Vector3D LookAt;
	Vector3D UpDir { 0.0f, 1.0f, 0.0f };
	Vector3D Forward;

	Matrix4x4 ProjectionMat;
	Matrix4x4 ViewMat;

	float FOV { 60.0f };
	float AspectRatio { 0.0f };
	float NearCull { 0.01f };
	float FarCull { 1000.0f };

	bool bIsActiveCamera { false };

	virtual void OnEditorInspect(Apollo::EntityID entityID) override;
	virtual void Load(const rapidjson::Value& data) override;
};