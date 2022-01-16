#pragma once

#include <Utils/Math/Vector3D.h>

struct VelocityComponent
{
	VelocityComponent() {}
	VelocityComponent(const Vector3D& velocity)
		: Velocity(velocity) {}

	Vector3D Velocity;
};