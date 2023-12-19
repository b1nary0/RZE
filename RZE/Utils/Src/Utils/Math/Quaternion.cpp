#include <StdAfx.h>
#include <Utils/Math/Quaternion.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Vector3D.h>

Quaternion::Quaternion()
	: m_quat(Vector3D().GetInternalVec())
{
}

Quaternion::Quaternion(const float x, const float y, const float z, const float w)
	: m_quat(w, x, y, z)
{
}

Quaternion::Quaternion(const Vector3D& angles)
{
	m_quat = glm::quat(angles.GetInternalVec());
}

Quaternion::Quaternion(const glm::quat& quat)
{
	m_quat = quat;
}

Quaternion::Quaternion(const Vector3D& a, const Vector3D& b)
	: m_quat(a.GetInternalVec(), b.GetInternalVec())
{
}

float Quaternion::ToAngle() const
{
	return glm::angle(m_quat);
}

Vector3D Quaternion::ToAxis() const
{
	glm::vec3 axis = glm::axis(m_quat);
	return Vector3D(axis.x, axis.y, axis.z);
}

Vector3D Quaternion::ToEuler() const
{
	glm::vec3 eulerRotation = glm::eulerAngles(m_quat);
	return Vector3D(eulerRotation.x, eulerRotation.y, eulerRotation.z) * MathUtils::ToDegrees;
}

const glm::quat& Quaternion::GetInternalQuat() const
{
	return m_quat;
}

Quaternion Quaternion::operator+(const Quaternion& rhs)
{
	return Quaternion(m_quat + rhs.m_quat);
}

void Quaternion::operator+=(const Quaternion& rhs)
{
	m_quat += rhs.GetInternalQuat();
}

Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
	return Quaternion(m_quat * rhs.GetInternalQuat());
}

void Quaternion::operator*=(const Quaternion& rhs)
{
	m_quat *= rhs.GetInternalQuat();
}

Vector3D Quaternion::operator*(const Vector3D& rhs) const
{
	glm::vec4 vec(rhs.GetInternalVec(), 1.0f);
	vec = m_quat * vec;

	return Vector3D(vec.x, vec.y, vec.z);
}
