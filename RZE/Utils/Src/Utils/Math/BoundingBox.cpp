#include <StdAfx.h>
#include <Utils/Math/BoundingBox.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector4D.h>

#include <cfloat>
#include <cmath>

BoundingBox::BoundingBox()
	: m_min(FLT_MAX)
	, m_max(-FLT_MAX)
{
}

BoundingBox::BoundingBox(const Vector3D& min, const Vector3D& max)
	: m_min(min)
	, m_max(max)
{
}

BoundingBox BoundingBox::Around(const Corners& points)
{
	BoundingBox box;
	for (const Vector3D& point : points)
	{
		box.Grow(point);
	}
	return box;
}

bool BoundingBox::IsEmpty() const
{
	return m_min.X() > m_max.X() || m_min.Y() > m_max.Y() || m_min.Z() > m_max.Z();
}

Vector3D BoundingBox::GetCentre() const
{
	return (m_min + m_max) * 0.5f;
}

Vector3D BoundingBox::GetSize() const
{
	return m_max - m_min;
}

float BoundingBox::GetRadius() const
{
	return GetSize().Length() * 0.5f;
}

BoundingBox::Corners BoundingBox::GetCorners() const
{
	Corners corners;
	for (int corner = 0; corner < 8; ++corner)
	{
		corners[corner] = Vector3D(
			(corner & 1) ? m_max.X() : m_min.X(),
			(corner & 2) ? m_max.Y() : m_min.Y(),
			(corner & 4) ? m_max.Z() : m_min.Z());
	}
	return corners;
}

BoundingBox::Corners BoundingBox::GetTransformedCorners(const Matrix4x4& transform) const
{
	Corners corners = GetCorners();
	for (Vector3D& corner : corners)
	{
		corner = (transform * Vector4D(corner, 1.0f)).XYZ();
	}
	return corners;
}

BoundingBox BoundingBox::Transformed(const Matrix4x4& transform) const
{
	if (IsEmpty())
	{
		return BoundingBox();
	}

	// Arvo's method: the centre moves with the transform, and the box reaches as far along each world axis as its
	// own half-size, projected onto that axis, takes it
	const Vector3D centre = (transform * Vector4D(GetCentre(), 1.0f)).XYZ();
	const Vector3D halfSize = GetSize() * 0.5f;
	float worldHalfSize[3];
	for (int axis = 0; axis < 3; ++axis)
	{
		const Vector4D row = transform.GetRow(axis);
		worldHalfSize[axis] = std::abs(row.X()) * halfSize.X() + std::abs(row.Y()) * halfSize.Y() + std::abs(row.Z()) * halfSize.Z();
	}

	const Vector3D reach(worldHalfSize[0], worldHalfSize[1], worldHalfSize[2]);
	return BoundingBox(centre - reach, centre + reach);
}

void BoundingBox::Grow(const Vector3D& point)
{
	m_min = VectorUtils::Min(m_min, point);
	m_max = VectorUtils::Max(m_max, point);
}

void BoundingBox::Grow(const BoundingBox& box)
{
	if (box.IsEmpty())
	{
		return;
	}

	m_min = VectorUtils::Min(m_min, box.m_min);
	m_max = VectorUtils::Max(m_max, box.m_max);
}

bool BoundingBox::operator==(const BoundingBox& other) const
{
	return m_min == other.m_min && m_max == other.m_max;
}
