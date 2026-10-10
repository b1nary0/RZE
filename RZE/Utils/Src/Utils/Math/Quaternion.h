#pragma once

#include <GLM/gtc/quaternion.hpp>

class Vector3D;

class Quaternion
{
public:
	Quaternion();
	Quaternion(const float x, const float y, const float z, const float w);
	// Euler angles in radians, applied X then Y then Z (see Units.h)
	Quaternion(const Vector3D& eulerRadians);
	// The shortest rotation taking unit vector a to unit vector b
	Quaternion(const Vector3D& a, const Vector3D& b);

	float ToAngleRadians() const;
	Vector3D ToEulerDegrees() const;
	Vector3D ToAxis() const;

	const glm::quat& GetInternalQuat() const;

	Quaternion operator+(const Quaternion& rhs);
	void operator+=(const Quaternion& rhs);

	Quaternion operator*(const Quaternion& rhs) const;
	Vector3D operator*(const Vector3D& rhs) const;
	void operator*=(const Quaternion& rhs);

private:
	Quaternion(const glm::quat& quat);

	glm::quat m_quat;
};
