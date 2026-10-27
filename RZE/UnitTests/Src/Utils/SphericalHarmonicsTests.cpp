#include <doctest/doctest.h>

#include <Utils/Math/Math.h>
#include <Utils/Math/SphericalHarmonics.h>
#include <Utils/Math/Vector3D.h>

#include <cmath>
#include <functional>

namespace
{
	constexpr int k_sampleCount = 4096;

	// Evenly spread over the sphere (a Fibonacci spiral), each standing for the same solid angle
	Vector3D SphereSample(int index)
	{
		const float goldenAngle = MathUtils::Pi * (3.0f - std::sqrt(5.0f));
		const float y = 1.0f - 2.0f * (static_cast<float>(index) + 0.5f) / static_cast<float>(k_sampleCount);
		const float radius = std::sqrt(1.0f - y * y);
		const float angle = goldenAngle * static_cast<float>(index);
		return Vector3D(std::cos(angle) * radius, y, std::sin(angle) * radius);
	}

	SphericalHarmonicsL1 Project(const std::function<float(const Vector3D&)>& environment)
	{
		const float solidAngle = 4.0f * MathUtils::Pi / static_cast<float>(k_sampleCount);
		SphericalHarmonicsL1 harmonics;
		for (int sample = 0; sample < k_sampleCount; ++sample)
		{
			const Vector3D direction = SphereSample(sample);
			harmonics.AddSample(direction, environment(direction), solidAngle);
		}
		return harmonics;
	}
}

TEST_CASE("A constant environment projects to itself and lights every direction the same")
{
	const SphericalHarmonicsL1 harmonics = Project([](const Vector3D&) { return 0.7f; });

	for (const Vector3D& direction : { Vector3D(1.0f, 0.0f, 0.0f), Vector3D(0.0f, -1.0f, 0.0f), Vector3D(0.0f, 0.6f, 0.8f) })
	{
		CHECK(harmonics.Evaluate(direction) == doctest::Approx(0.7f).epsilon(0.001));
		CHECK(harmonics.EvaluateIrradiance(direction) == doctest::Approx(0.7f).epsilon(0.001));
	}
}

TEST_CASE("A sky above and dark ground below light up-facing surfaces as the cosine-weighted average")
{
	// Radiance 1 from above the horizon, 0 below
	const SphericalHarmonicsL1 harmonics = Project([](const Vector3D& direction) { return direction.Y() > 0.0f ? 1.0f : 0.0f; });

	// A surface facing up sees only sky (1), facing down only ground (0), sideways half of each (0.5). L1 gets all three
	// exactly: 0.5 from band 0, plus or minus 0.5 from band 1 facing up or down.
	CHECK(harmonics.EvaluateIrradiance(Vector3D(1.0f, 0.0f, 0.0f)) == doctest::Approx(0.5f).epsilon(0.002));
	CHECK(harmonics.EvaluateIrradiance(Vector3D(0.0f, 1.0f, 0.0f)) == doctest::Approx(1.0f).epsilon(0.002));
	CHECK(harmonics.EvaluateIrradiance(Vector3D(0.0f, -1.0f, 0.0f)) == doctest::Approx(0.0f).epsilon(0.002));
}

TEST_CASE("Light from one direction is brightest facing it and darkest facing away")
{
	const Vector3D lightDirection = Vector3D(0.3f, 0.4f, -0.866f).Normalized();
	const float solidAngle = 0.01f;
	SphericalHarmonicsL1 harmonics;
	harmonics.AddSample(lightDirection, 1.0f, solidAngle);

	const float facing = harmonics.EvaluateIrradiance(lightDirection);
	const float side = harmonics.EvaluateIrradiance(lightDirection.Cross(Vector3D(0.0f, 1.0f, 0.0f)).Normalized());
	const float away = harmonics.EvaluateIrradiance(lightDirection * -1.0f);
	CHECK(facing > side);
	CHECK(side > away);

	// L1's version of max(cos, 0) / pi: (1 + 2 cos) / 4pi, so 3 / 4pi facing the light
	CHECK(facing == doctest::Approx(solidAngle * 3.0f / (4.0f * MathUtils::Pi)).epsilon(0.0001));
	CHECK(side == doctest::Approx(solidAngle / (4.0f * MathUtils::Pi)).epsilon(0.0001));
}
