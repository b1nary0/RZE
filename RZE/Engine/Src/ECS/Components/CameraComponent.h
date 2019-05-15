#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

struct CameraComponent final : public Apollo::Component<CameraComponent>
{
	Vector3D LookAt;
	Vector3D UpDir;
	Vector3D Forward;

	Matrix4x4 ProjectionMat;
	Matrix4x4 ViewMat;

	float FOV { 0.0f };
	float AspectRatio { 0.0f };
	float NearCull { 0.0f };
	float FarCull { 0.0f };

	bool bIsActiveCamera { false };

	void OnEditorInspect() override;
};