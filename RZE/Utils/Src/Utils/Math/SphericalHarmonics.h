#pragma once

#include <array>

class Vector3D;

// A function over directions, such as the light arriving at a point from every direction, smoothed down to order 1
// (L1): a constant plus one linear term per axis, 4 coefficients. Enough for diffuse light, which blurs out anything
// finer. The irradiance probes keep one per colour channel; their shaders mirror these conventions.
class SphericalHarmonicsL1
{
public:
	static constexpr int k_coefficientCount = 4;

public:
	// Adds value arriving from direction (unit length), standing for solidAngle steradians of the sphere. Samples that
	// cover the sphere once add up to its projection.
	void AddSample(const Vector3D& direction, float value, float solidAngle);

	// The smoothed function towards direction (unit length)
	float Evaluate(const Vector3D& direction) const;

	// The function averaged over the hemisphere around normal (unit length), weighted by the cosine to it: what a white
	// diffuse surface facing normal reflects. A constant function gives itself back. (Irradiance / pi.)
	float EvaluateIrradiance(const Vector3D& normal) const;

	// Y00, then Y1-1, Y10, Y11, which go with y, z and x
	const std::array<float, k_coefficientCount>& GetCoefficients() const { return m_coefficients; }

private:
	std::array<float, k_coefficientCount> m_coefficients{};
};
