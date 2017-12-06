#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

struct CameraComponent : public Apollo::Component<CameraComponent>
{
	Vector3D Position;
	Vector3D LookAt;
	Vector3D UpDir;
	Vector3D Forward;

	Matrix4x4 ProjectionMat;
	Matrix4x4 ViewMat;

	float FOV;
	float AspectRatio;
	float NearCull;
	float FarCull;
};