#pragma once

#include <Utils/Math/Plane.h>

class BoundingBox;
class Matrix4x4;

// The six planes of a camera's view volume, for testing what the camera can see. Each plane's normal
// points into the volume.
class Frustum
{
public:
	// Extracts the planes from a view-projection matrix (Gribb & Hartmann). Assumes D3D's 0..1 clip-space
	// depth, as the camera projections produce (Matrix4x4::CreatePerspectiveMatrixZeroToOne).
	explicit Frustum(const Matrix4x4& viewProjection);

	// False only when the box is entirely outside one of the planes, or empty. A box just outside a corner of the
	// frustum can still return true, which is fine for culling.
	bool Intersects(const BoundingBox& box) const;

private:
	Plane m_planes[6];
};
