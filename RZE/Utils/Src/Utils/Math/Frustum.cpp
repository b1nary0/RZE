#include <StdAfx.h>
#include <Utils/Math/Frustum.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

Frustum::Frustum(const Matrix4x4& viewProjection)
{
	const Vector4D row0 = viewProjection.GetRow(0);
	const Vector4D row1 = viewProjection.GetRow(1);
	const Vector4D row2 = viewProjection.GetRow(2);
	const Vector4D row3 = viewProjection.GetRow(3);

	m_planes[0] = Plane(row3 + row0); // Left
	m_planes[1] = Plane(row3 - row0); // Right
	m_planes[2] = Plane(row3 + row1); // Bottom
	m_planes[3] = Plane(row3 - row1); // Top
	m_planes[4] = Plane(row2);        // Near: z >= 0
	m_planes[5] = Plane(row3 - row2); // Far
}

bool Frustum::Intersects(const BoundingBox& box) const
{
	if (box.IsEmpty())
	{
		return false;
	}

	const Vector3D& boxMin = box.GetMin();
	const Vector3D& boxMax = box.GetMax();
	for (const Plane& plane : m_planes)
	{
		// The box corner furthest along the plane's normal
		const Vector3D normal = plane.GetNormal();
		const Vector3D furthest(
			normal.X() >= 0.0f ? boxMax.X() : boxMin.X(),
			normal.Y() >= 0.0f ? boxMax.Y() : boxMin.Y(),
			normal.Z() >= 0.0f ? boxMax.Z() : boxMin.Z());

		if (plane.SignedDistance(furthest) < 0.0f)
		{
			return false;
		}
	}

	return true;
}
