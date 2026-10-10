#include <StdAfx.h>
#include <Utils/Math/Plane.h>

#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <GLM/geometric.hpp>

Plane::Plane()
	: m_plane(0.0f, 1.0f, 0.0f, 0.0f)
{
}

Plane::Plane(const Vector3D& normal, const float distance)
	: m_plane(normal.GetInternalVec(), distance)
{
}

Plane::Plane(const Vector4D& coefficients)
	: m_plane(coefficients.GetInternalVec())
{
}

Vector3D Plane::GetNormal() const
{
	return Vector3D(m_plane.x, m_plane.y, m_plane.z);
}

float Plane::GetDistance() const
{
	return m_plane.w;
}

float Plane::SignedDistance(const Vector3D& point) const
{
	return glm::dot(glm::vec3(m_plane), point.GetInternalVec()) + m_plane.w;
}
