#pragma once

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Plane.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

// The six planes of a camera's view volume, for testing what the camera can see. Each plane's normal
// points into the volume.
class Frustum
{
public:
	// Extracts the planes from a view-projection matrix (Gribb & Hartmann). Assumes GLM's default -1..1
	// clip-space depth; with a 0..1 projection the near plane would sit slightly behind the real one,
	// which only makes culling more conservative.
	explicit Frustum(const Matrix4x4& viewProjection)
	{
		const Vector4D row0 = viewProjection.GetRow(0);
		const Vector4D row1 = viewProjection.GetRow(1);
		const Vector4D row2 = viewProjection.GetRow(2);
		const Vector4D row3 = viewProjection.GetRow(3);

		m_planes[0] = Plane(row3 + row0); // Left
		m_planes[1] = Plane(row3 - row0); // Right
		m_planes[2] = Plane(row3 + row1); // Bottom
		m_planes[3] = Plane(row3 - row1); // Top
		m_planes[4] = Plane(row3 + row2); // Near
		m_planes[5] = Plane(row3 - row2); // Far
	}

	// False only when the box is entirely outside one of the planes. A box just outside a corner of the
	// frustum can still return true, which is fine for culling.
	bool Intersects(const Vector3D& boxMin, const Vector3D& boxMax) const
	{
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

private:
	Plane m_planes[6];
};
