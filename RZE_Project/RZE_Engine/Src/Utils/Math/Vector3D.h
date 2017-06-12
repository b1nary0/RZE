#pragma once

#include <GLM/vec3.hpp>

class Vector3D
{
public:

	Vector3D();
	Vector3D(const float x, const float y, const float z);

	float X() const;
	float Y() const;
	float Z() const;

	const glm::vec3 GetInternalVec() const;

	Vector3D operator+(const Vector3D& rhs);

private:
	glm::vec3 mVec;
};
