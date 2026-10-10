#pragma once

#include <GLM/vec4.hpp>

class Vector3D;
class Vector4D;

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

private:
	// (normal.x, normal.y, normal.z, distance)
	glm::vec4 m_plane;
};
