#include <doctest/doctest.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Frustum.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

namespace
{
	// At the origin looking along -z, seeing from 0.1 m to 100 m
	Frustum MakeFrustum()
	{
		const Matrix4x4 view = Matrix4x4::CreateViewMatrix(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, -1.0f), Vector3D(0.0f, 1.0f, 0.0f));
		const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(60.0f, 1.0f, 0.1f, 100.0f);
		return Frustum(projection * view);
	}

	BoundingBox BoxAt(const Vector3D& centre)
	{
		return BoundingBox(centre - Vector3D(0.5f, 0.5f, 0.5f), centre + Vector3D(0.5f, 0.5f, 0.5f));
	}
}

TEST_CASE("A frustum sees boxes in front of the camera, and not ones outside its planes")
{
	const Frustum frustum = MakeFrustum();

	CHECK(frustum.Intersects(BoxAt(Vector3D(0.0f, 0.0f, -10.0f))));
	// Partly inside counts
	CHECK(frustum.Intersects(BoxAt(Vector3D(0.0f, 0.0f, -100.0f))));

	CHECK_FALSE(frustum.Intersects(BoxAt(Vector3D(0.0f, 0.0f, 10.0f))));     // behind
	CHECK_FALSE(frustum.Intersects(BoxAt(Vector3D(0.0f, 0.0f, -200.0f))));   // past the far plane
	CHECK_FALSE(frustum.Intersects(BoxAt(Vector3D(50.0f, 0.0f, -10.0f))));   // off to the right
	CHECK_FALSE(frustum.Intersects(BoxAt(Vector3D(0.0f, -50.0f, -10.0f))));  // below
}

TEST_CASE("A frustum never sees an empty box")
{
	CHECK_FALSE(MakeFrustum().Intersects(BoundingBox()));
}
