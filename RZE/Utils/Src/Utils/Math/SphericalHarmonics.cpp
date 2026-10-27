#include <StdAfx.h>
#include <Utils/Math/SphericalHarmonics.h>

#include <Utils/Math/Vector3D.h>

namespace
{
	// The real spherical harmonics' normalisation: sqrt(1 / 4pi) for band 0, sqrt(3 / 4pi) for band 1
	constexpr float k_band0 = 0.282094792f;
	constexpr float k_band1 = 0.488602512f;

	// Convolving with the clamped cosine scales band 0 by pi and band 1 by 2pi / 3 (Ramamoorthi and Hanrahan 2001,
	// "An Efficient Representation for Irradiance Environment Maps"); divided by pi, since the result is irradiance / pi
	constexpr float k_irradianceBand0 = 1.0f;
	constexpr float k_irradianceBand1 = 2.0f / 3.0f;

	std::array<float, SphericalHarmonicsL1::k_coefficientCount> Basis(const Vector3D& direction)
	{
		return { k_band0, k_band1 * direction.Y(), k_band1 * direction.Z(), k_band1 * direction.X() };
	}
}

void SphericalHarmonicsL1::AddSample(const Vector3D& direction, float value, float solidAngle)
{
	const std::array<float, k_coefficientCount> basis = Basis(direction);
	for (int coefficient = 0; coefficient < k_coefficientCount; ++coefficient)
	{
		m_coefficients[coefficient] += value * basis[coefficient] * solidAngle;
	}
}

float SphericalHarmonicsL1::Evaluate(const Vector3D& direction) const
{
	const std::array<float, k_coefficientCount> basis = Basis(direction);
	float value = 0.0f;
	for (int coefficient = 0; coefficient < k_coefficientCount; ++coefficient)
	{
		value += m_coefficients[coefficient] * basis[coefficient];
	}
	return value;
}

float SphericalHarmonicsL1::EvaluateIrradiance(const Vector3D& normal) const
{
	const std::array<float, k_coefficientCount> basis = Basis(normal);
	float value = m_coefficients[0] * basis[0] * k_irradianceBand0;
	for (int coefficient = 1; coefficient < k_coefficientCount; ++coefficient)
	{
		value += m_coefficients[coefficient] * basis[coefficient] * k_irradianceBand1;
	}
	return value;
}
