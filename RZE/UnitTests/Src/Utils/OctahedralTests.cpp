#include <doctest/doctest.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/Octahedral.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <cmath>

TEST_CASE("Directions come back from the octahedral square as they went in")
{
	constexpr int k_sampleCount = 1000;
	const float goldenAngle = MathUtils::Pi * (3.0f - std::sqrt(5.0f));
	for (int sample = 0; sample < k_sampleCount; ++sample)
	{
		const float z = 1.0f - 2.0f * (static_cast<float>(sample) + 0.5f) / static_cast<float>(k_sampleCount);
		const float radius = std::sqrt(1.0f - z * z);
		const float angle = goldenAngle * static_cast<float>(sample);
		// Normalized: at angles this large (up to ~2400 rad), MSVC's optimized float sin and cos drift far enough that
		// the point comes out up to 2e-5 off the unit sphere, which is the test's error, not Encode's or Decode's
		const Vector3D direction = Vector3D(std::cos(angle) * radius, std::sin(angle) * radius, z).Normalized();

		const Vector2D point = Octahedral::Encode(direction);
		CHECK(std::abs(point.X()) <= 1.0f);
		CHECK(std::abs(point.Y()) <= 1.0f);

		const Vector3D decoded = Octahedral::Decode(point);
		CHECK(VectorUtils::Dot(decoded, direction) == doctest::Approx(1.0f).epsilon(0.00001));
	}
}

TEST_CASE("The octahedral square has +Z in the middle, -Z in the corners and the equator on its diamond")
{
	const Vector2D up = Octahedral::Encode(Vector3D(0.0f, 0.0f, 1.0f));
	CHECK(up.X() == doctest::Approx(0.0f));
	CHECK(up.Y() == doctest::Approx(0.0f));

	const Vector2D down = Octahedral::Encode(Vector3D(0.0f, 0.0f, -1.0f));
	CHECK(std::abs(down.X()) == doctest::Approx(1.0f));
	CHECK(std::abs(down.Y()) == doctest::Approx(1.0f));
	for (const Vector2D& corner : { Vector2D(1.0f, 1.0f), Vector2D(-1.0f, 1.0f), Vector2D(1.0f, -1.0f), Vector2D(-1.0f, -1.0f) })
	{
		CHECK(Octahedral::Decode(corner).Z() == doctest::Approx(-1.0f));
	}

	const Vector2D east = Octahedral::Encode(Vector3D(1.0f, 0.0f, 0.0f));
	CHECK(east.X() == doctest::Approx(1.0f));
	CHECK(east.Y() == doctest::Approx(0.0f));
}

TEST_CASE("The octahedral square's edges meet themselves mirrored, so a map's border copies across them")
{
	// What Compute_ProbeVisibility's border relies on: a left or right edge is the same direction at v and -v, the top
	// or bottom at u and -u
	for (float t = -1.0f; t <= 1.0f; t += 0.125f)
	{
		for (const float edge : { -1.0f, 1.0f })
		{
			const Vector3D side = Octahedral::Decode(Vector2D(edge, t));
			const Vector3D sideMirrored = Octahedral::Decode(Vector2D(edge, -t));
			CHECK(VectorUtils::Dot(side, sideMirrored) == doctest::Approx(1.0f).epsilon(0.00001));

			const Vector3D topOrBottom = Octahedral::Decode(Vector2D(t, edge));
			const Vector3D topOrBottomMirrored = Octahedral::Decode(Vector2D(-t, edge));
			CHECK(VectorUtils::Dot(topOrBottom, topOrBottomMirrored) == doctest::Approx(1.0f).epsilon(0.00001));
		}
	}
}
