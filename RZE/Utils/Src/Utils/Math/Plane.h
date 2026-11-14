#pragma once

#include <GLM/geometric.hpp>
#include <GLM/vec4.hpp>

#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

// The points p where Dot(normal, p) + distance == 0, i.e. ax + by + cz + d = 0.
//
// The normal doesn't have to be unit length. If it isn't, SignedDistance() is scaled by its length, but its
// sign (which side of the plane a point is on) is still correct, which is all side tests need.
class Plane
{
public:
	Plane();
	Plane(const Vector3D& normal, const float distance);
	// From the plane equation's coefficients (a, b, c, d)
	explicit Plane(const Vector4D& coefficients);

public:
	Vector3D GetNormal() const;
	float GetDistance() const;

	// Positive on the side the normal points to, negative on the other, zero on the plane
	float SignedDistance(const Vector3D& point) const;

	// Exactly the same coefficients
	bool operator==(const Plane& other) const;

private:
	// (normal.x, normal.y, normal.z, distance)
	glm::vec4 m_plane;
};

// The one-statement functions, here so callers in other .cpp files can inline them (no build uses LTO)

inline Plane::Plane()
	: m_plane(0.0f, 1.0f, 0.0f, 0.0f)
{
}

inline Plane::Plane(const Vector3D& normal, const float distance)
	: m_plane(normal.GetInternalVec(), distance)
{
}

inline Plane::Plane(const Vector4D& coefficients)
	: m_plane(coefficients.GetInternalVec())
{
}

inline Vector3D Plane::GetNormal() const
{
	return Vector3D(m_plane.x, m_plane.y, m_plane.z);
}

inline float Plane::GetDistance() const
{
	return m_plane.w;
}

inline float Plane::SignedDistance(const Vector3D& point) const
{
	return glm::dot(glm::vec3(m_plane), point.GetInternalVec()) + m_plane.w;
}

inline bool Plane::operator==(const Plane& other) const
{
	return m_plane == other.m_plane;
}
