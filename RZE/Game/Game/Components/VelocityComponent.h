#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <Utils/Math/Vector3D.h>

struct VelocityComponent : public Apollo::Component<VelocityComponent>
{
	VelocityComponent() {}
	VelocityComponent(const Vector3D& velocity)
		: Velocity(velocity) {}

	Vector3D Velocity;
};