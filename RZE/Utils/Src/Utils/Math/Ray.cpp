#include <StdAfx.h>
#include <Utils/Math/Ray.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Plane.h>

#include <cmath>

Ray::Ray(const Vector3D& origin, const Vector3D& direction)
	: m_origin(origin)
	, m_direction(direction.Normalized())
{
}

Vector3D Ray::GetPoint(float distance) const
{
	return m_origin + m_direction * distance;
}

std::optional<Vector3D> Ray::Intersect(const Plane& plane) const
{
	// How fast the ray closes on the plane per unit along it, in SignedDistance's units
	constexpr float k_minApproach = 1e-4f;
	const float approach = VectorUtils::Dot(plane.GetNormal(), m_direction);
	if (std::abs(approach) < k_minApproach)
	{
		return std::nullopt;
	}

	// SignedDistance is linear along the ray, so it reaches zero this far along
	const float distance = -plane.SignedDistance(m_origin) / approach;
	if (distance < 0.0f)
	{
		return std::nullopt;
	}

	return GetPoint(distance);
}
