#include <StdAfx.h>
#include <Graphics/IrradianceProbeGrid.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Math.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Probes along an extent padded by one spacing at each end, so the outermost surfaces have probes beyond them
	U32 CountAlong(float extent, float spacing)
	{
		const U32 count = static_cast<U32>(std::ceil((extent + 2.0f * spacing) / spacing)) + 1;
		return std::max(count, IrradianceProbeGrid::k_minProbesPerAxis);
	}
}

IrradianceProbeGrid IrradianceProbeGrid::Fit(const BoundingBox& bounds, U32 targetCount)
{
	if (bounds.IsEmpty() || targetCount == 0)
	{
		return {};
	}

	// A flat scene has no volume, so each extent counts as at least the smallest spacing
	const Vector3D size = bounds.GetSize();
	const float volume = std::max(size.X(), k_minSpacing) * std::max(size.Y(), k_minSpacing) * std::max(size.Z(), k_minSpacing);
	float spacing = MathUtils::Clampf(std::cbrt(volume / static_cast<float>(targetCount)), k_minSpacing, k_maxSpacing);

	// The padding and the probes at both ends add their own, nearly doubling a room-sized scene's count, so the spacing
	// is corrected by how far the real count is from the target. A few rounds get within 5%, or to a limit.
	IrradianceProbeGrid grid;
	for (int round = 0; round < 8; ++round)
	{
		grid.CountX = CountAlong(size.X(), spacing);
		grid.CountY = CountAlong(size.Y(), spacing);
		grid.CountZ = CountAlong(size.Z(), spacing);
		const float ratio = static_cast<float>(grid.GetProbeCount()) / static_cast<float>(targetCount);
		if (std::abs(ratio - 1.0f) < 0.05f)
		{
			break;
		}

		const float corrected = MathUtils::Clampf(spacing * std::cbrt(ratio), k_minSpacing, k_maxSpacing);
		if (corrected == spacing)
		{
			break;
		}
		spacing = corrected;
	}

	for (;;)
	{
		grid.CountX = CountAlong(size.X(), spacing);
		grid.CountY = CountAlong(size.Y(), spacing);
		grid.CountZ = CountAlong(size.Z(), spacing);
		if (grid.GetProbeCount() <= k_maxProbes)
		{
			break;
		}

		// Over the cap (the padding and rounding up can push it there, or a scene too big for k_maxSpacing): spread
		// the probes out by what's needed to bring the count down, and a little more so it converges
		spacing *= std::cbrt(static_cast<float>(grid.GetProbeCount()) / static_cast<float>(k_maxProbes)) * 1.01f;
	}

	grid.Spacing = spacing;
	grid.Origin = bounds.GetMin() - Vector3D(spacing);
	return grid;
}

Vector3D IrradianceProbeGrid::GetProbePosition(U32 x, U32 y, U32 z) const
{
	return Origin + Vector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * Spacing;
}

Vector3D IrradianceProbeGrid::GetProbePosition(U32 probe) const
{
	const U32 x = probe % CountX;
	const U32 y = (probe / CountX) % CountY;
	const U32 z = probe / (CountX * CountY);
	return GetProbePosition(x, y, z);
}
