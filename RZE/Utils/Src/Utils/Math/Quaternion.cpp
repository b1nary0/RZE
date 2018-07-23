#include <Utils/StdAfx.h>
#include <Utils/Math/Quaternion.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Vector3D.h>

Quaternion::Quaternion()
	: mQuat(Vector3D().GetInternalVec())
{
}

Quaternion::Quaternion(const float x, const float y, const float z, const float w)
	: mQuat(x, y, z, w)
{
}

Quaternion::Quaternion(const Vector3D& angles)
{
	Vector3D anglesInRadians(angles.X() * MathUtils::ToRadians, angles.Y() * MathUtils::ToRadians, angles.Z() * MathUtils::ToRadians);
	mQuat = anglesInRadians.GetInternalVec();
}

Quaternion::Quaternion(const glm::quat& quat)
{
	mQuat = quat;
}

Quaternion::Quaternion(const Vector3D& a, const Vector3D& b)
	: mQuat(a.GetInternalVec(), b.GetInternalVec())
{
}

float Quaternion::ToAngle() const
{
	return glm::angle(mQuat);
}

Vector3D Quaternion::ToAxis() const
{
	glm::vec3 axis = glm::axis(mQuat);
	return Vector3D(axis.x, axis.y, axis.z);
}

const glm::quat& Quaternion::GetInternalQuat() const
{
	return mQuat;
}

Quaternion Quaternion::operator+(const Quaternion& rhs)
{
	return Quaternion(mQuat + rhs.mQuat);
}

void Quaternion::operator+=(const Quaternion& rhs)
{
	mQuat += rhs.GetInternalQuat();
}

Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
	return Quaternion(mQuat * rhs.GetInternalQuat());
}

void Quaternion::operator*=(const Quaternion& rhs)
{
	mQuat *= rhs.GetInternalQuat();
}

Vector3D Quaternion::operator*(const Vector3D& rhs) const
{
	glm::vec4 vec(rhs.GetInternalVec(), 1.0f);
	vec = mQuat * vec;

	return Vector3D(vec.x, vec.y, vec.z);
}
