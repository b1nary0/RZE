#pragma once

#include <Utils/Math/Vector3D.h>

#include <optional>

class Plane;

// A half-line: from an origin, out along a direction
class Ray
{
public:
	// direction is normalised; it mustn't be zero
	Ray(const Vector3D& origin, const Vector3D& direction);

public:
	const Vector3D& GetOrigin() const { return m_origin; }
	// Unit length
	const Vector3D& GetDirection() const { return m_direction; }

	// distance along the ray from its origin
	Vector3D GetPoint(float distance) const;

	// Where the ray meets plane, if it does ahead of its origin. Never for a ray (nearly) parallel to the plane.
	std::optional<Vector3D> Intersect(const Plane& plane) const;

private:
	Vector3D m_origin;
	Vector3D m_direction;
};
