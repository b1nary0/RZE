#pragma once

#include <Utils/Math/Ray.h>

class Matrix4x4;
class Vector2D;
class Vector3D;

// From a view's pixels back to the world. clipSpace is the view's projection * view, with D3D's zero-to-one depth as
// the camera projections give (Matrix4x4::CreatePerspectiveMatrixZeroToOne). pixel is from the view's top-left, with y
// running down, so a pixel's centre is at +0.5.
namespace ViewProjection
{
	// The world position drawn at pixel with depth (0 at the near plane, 1 at the far)
	Vector3D Unproject(const Matrix4x4& clipSpace, const Vector2D& viewSize, const Vector2D& pixel, float depth);
	// From the near plane, through pixel
	Ray GetRay(const Matrix4x4& clipSpace, const Vector2D& viewSize, const Vector2D& pixel);
}
