#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Vector3D.h>

struct TransformComponent : public Apollo::Component<TransformComponent>
{
	TransformComponent() = default;
	
	explicit TransformComponent(const Vector3D& position)
	{
		Transform.SetPosition(position);
	}

	TransformComponent(const Vector3D& position, const Quaternion& rotation)
	{
		Transform.SetPosition(position);
		Transform.SetRotation(rotation);
	}

	TransformComponent(const Vector3D& position, const Quaternion& rotation, const Vector3D& scale)
	{
		Transform.SetPosition(position);
		Transform.SetRotation(rotation);
		Transform.SetScale(scale);
	}

	Matrix4x4 Transform;

 	//Vector3D Scale {1, 1, 1};
	Quaternion Rotation;
};