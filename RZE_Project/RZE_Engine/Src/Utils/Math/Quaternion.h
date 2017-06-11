#pragma once

#include <GLM/gtc/quaternion.hpp>

class Vector3D;

class Quaternion
{
public:
	Quaternion();
	Quaternion(const Vector3D& angles);

	float ToAngle() const;
	Vector3D ToAxis() const;

	const glm::quat& GetInternalQuat();

private:
	glm::quat mQuat;
};
