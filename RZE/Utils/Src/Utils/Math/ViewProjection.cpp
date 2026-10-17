#include <StdAfx.h>
#include <Utils/Math/ViewProjection.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace ViewProjection
{
	Vector3D Unproject(const Matrix4x4& clipSpace, const Vector2D& viewSize, const Vector2D& pixel, float depth)
	{
		// Screen y runs down, NDC y runs up
		const float ndcX = pixel.X() / viewSize.X() * 2.0f - 1.0f;
		const float ndcY = 1.0f - pixel.Y() / viewSize.Y() * 2.0f;

		// The inverse gives back the point scaled by 1/w, the perspective divide's w; dividing by its own w undoes that
		const Vector4D world = clipSpace.Inverse() * Vector4D(ndcX, ndcY, depth, 1.0f);
		return world.XYZ() / world.W();
	}

	Ray GetRay(const Matrix4x4& clipSpace, const Vector2D& viewSize, const Vector2D& pixel)
	{
		const Vector3D nearPoint = Unproject(clipSpace, viewSize, pixel, 0.0f);
		const Vector3D farPoint = Unproject(clipSpace, viewSize, pixel, 1.0f);
		return Ray(nearPoint, farPoint - nearPoint);
	}
}
