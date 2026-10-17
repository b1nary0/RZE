#include <doctest/doctest.h>

#include <Utils/Math/Plane.h>
#include <Utils/Math/Ray.h>
#include <Utils/Math/Vector3D.h>

namespace
{
	void CheckNear(const Vector3D& actual, const Vector3D& expected)
	{
		CHECK(actual.X() == doctest::Approx(expected.X()));
		CHECK(actual.Y() == doctest::Approx(expected.Y()));
		CHECK(actual.Z() == doctest::Approx(expected.Z()));
	}
}

TEST_CASE("A ray's direction is unit length, and GetPoint goes that far along it")
{
	const Ray ray(Vector3D(1.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, -5.0f));
	CheckNear(ray.GetDirection(), Vector3D(0.0f, 0.0f, -1.0f));
	CheckNear(ray.GetPoint(3.0f), Vector3D(1.0f, 0.0f, -3.0f));
}

TEST_CASE("A ray meets a plane ahead of it, from either side")
{
	const Plane ground(Vector3D(0.0f, 1.0f, 0.0f), 0.0f);

	const std::optional<Vector3D> fromAbove = Ray(Vector3D(0.0f, 2.0f, 10.0f), Vector3D(0.0f, -1.0f, -1.0f)).Intersect(ground);
	REQUIRE(fromAbove.has_value());
	CheckNear(*fromAbove, Vector3D(0.0f, 0.0f, 8.0f));

	CHECK(Ray(Vector3D(0.0f, -1.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f)).Intersect(ground).has_value());

	// A plane not through the origin, with a normal that isn't unit length: y = 3
	const Plane raised(Vector3D(0.0f, 2.0f, 0.0f), -6.0f);
	const std::optional<Vector3D> hit = Ray(Vector3D(1.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f)).Intersect(raised);
	REQUIRE(hit.has_value());
	CheckNear(*hit, Vector3D(1.0f, 3.0f, 0.0f));
}

TEST_CASE("A ray heading away from a plane, or along it, never meets it")
{
	const Plane ground(Vector3D(0.0f, 1.0f, 0.0f), 0.0f);
	CHECK_FALSE(Ray(Vector3D(0.0f, 2.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f)).Intersect(ground).has_value());
	CHECK_FALSE(Ray(Vector3D(0.0f, 2.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f)).Intersect(ground).has_value());
}
