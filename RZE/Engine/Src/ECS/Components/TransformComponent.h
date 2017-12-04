#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Quaternion.h>

struct TransformComponent : public Apollo::Component<TransformComponent>
{
	TransformComponent() = default;
	
	explicit TransformComponent(const Vector3D& position)
		: Position(position)
	{}

	TransformComponent(const Vector3D& position, const Quaternion& rotation)
		: Position(position)
		, Rotation(rotation)
	{}

	TransformComponent(const Vector3D& position, const Quaternion& rotation, const Vector3D& scale)
		: Position(position)
		, Rotation(rotation)
		, Scale(scale)
	{}

	Vector3D Position;
	Vector3D Scale;
	Quaternion Rotation;
};