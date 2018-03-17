#pragma once

#include <GLM/gtc/quaternion.hpp>

class Vector3D;

class Quaternion
{
public:
	Quaternion();
	Quaternion(const float x, const float y, const float z, const float w);
	Quaternion(const Vector3D& angles);
	Quaternion(const Vector3D& a, const Vector3D& b);

	float ToAngle() const;
	Vector3D ToAxis() const;

	const glm::quat& GetInternalQuat() const;

	Quaternion operator+(const Quaternion& rhs);
	void operator+=(const Quaternion& rhs);

	Quaternion operator*(const Quaternion& rhs) const;
	Vector3D operator*(const Vector3D& rhs) const;
	void operator*=(const Quaternion& rhs);

private:
	Quaternion(const glm::quat& quat);

	glm::quat mQuat;
};
