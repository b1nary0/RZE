#include <doctest/doctest.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Math/ViewProjection.h>

namespace
{
	const Vector2D k_viewSize(1600.0f, 900.0f);

	// 2 m up, 10 m back from the origin, looking along -z
	Matrix4x4 MakeClipSpace()
	{
		const Vector3D eye(0.0f, 2.0f, 10.0f);
		const Matrix4x4 view = Matrix4x4::CreateViewMatrix(eye, eye + Vector3D(0.0f, 0.0f, -1.0f), Vector3D(0.0f, 1.0f, 0.0f));
		const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(60.0f, k_viewSize.X() / k_viewSize.Y(), 0.1f, 1000.0f);
		return projection * view;
	}

	void CheckNear(const Vector3D& actual, const Vector3D& expected, double epsilon)
	{
		CHECK(actual.X() == doctest::Approx(expected.X()).epsilon(epsilon));
		CHECK(actual.Y() == doctest::Approx(expected.Y()).epsilon(epsilon));
		CHECK(actual.Z() == doctest::Approx(expected.Z()).epsilon(epsilon));
	}
}

TEST_CASE("Unproject gives back the world position drawn at a pixel and depth")
{
	const Matrix4x4 clipSpace = MakeClipSpace();
	const Vector3D point(1.5f, 0.5f, 3.0f);

	// Where the point is drawn, and at what depth
	const Vector4D clip = clipSpace * Vector4D(point, 1.0f);
	const Vector2D pixel((clip.X() / clip.W() + 1.0f) * 0.5f * k_viewSize.X(), (1.0f - clip.Y() / clip.W()) * 0.5f * k_viewSize.Y());
	const float depth = clip.Z() / clip.W();

	CheckNear(ViewProjection::Unproject(clipSpace, k_viewSize, pixel, depth), point, 1e-3);
}

TEST_CASE("GetRay starts at the near plane and goes through the pixel")
{
	const Matrix4x4 clipSpace = MakeClipSpace();

	const Ray centre = ViewProjection::GetRay(clipSpace, k_viewSize, Vector2D(k_viewSize.X() * 0.5f, k_viewSize.Y() * 0.5f));
	CheckNear(centre.GetOrigin(), Vector3D(0.0f, 2.0f, 9.9f), 1e-3);
	CheckNear(centre.GetDirection(), Vector3D(0.0f, 0.0f, -1.0f), 1e-3);

	// Below the middle, the ray heads down
	const Ray lower = ViewProjection::GetRay(clipSpace, k_viewSize, Vector2D(k_viewSize.X() * 0.5f, k_viewSize.Y() * 0.9f));
	CHECK(lower.GetDirection().Y() < 0.0f);
	CHECK(lower.GetDirection().Length() == doctest::Approx(1.0f));
}
