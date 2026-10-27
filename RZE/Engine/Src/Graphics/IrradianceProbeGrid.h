#pragma once

#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

class BoundingBox;

// Where a scene's irradiance probes are: a regular grid over everything it draws, padded by one spacing all round.
// The spacing scales with the scene's size, so a tabletop scene gets as many probes as an atrium
// (Docs/Plans/SSAOAndGlobalIllumination.md, "The grid").
struct IrradianceProbeGrid
{
	// The spacing is kept within these unless the probe count would pass k_maxProbes, which wins
	static constexpr float k_minSpacing = 0.1f;
	static constexpr float k_maxSpacing = 4.0f;
	static constexpr U32 k_minProbesPerAxis = 2;
	// What the probe textures are made for: 8192 probes is 32 x 8 x 32
	static constexpr U32 k_maxProbes = 8192;

	// The probes from Min to Max along each axis (x, y, z), both included
	struct ProbeRange
	{
		U32 Min[3] = { 0, 0, 0 };
		U32 Max[3] = { 0, 0, 0 };
		bool IsEmpty = true;
	};

	// The probe with the lowest x, y and z
	Vector3D Origin;
	float Spacing = 0.0f;
	U32 CountX = 0;
	U32 CountY = 0;
	U32 CountZ = 0;

	// A grid over bounds (none for empty bounds) with about targetCount probes, padding included: the spacing starts at
	// cbrt(volume / targetCount) and is corrected by the count it gives, within the limits above
	static IrradianceProbeGrid Fit(const BoundingBox& bounds, U32 targetCount);

	U32 GetProbeCount() const { return CountX * CountY * CountZ; }
	bool IsEmpty() const { return GetProbeCount() == 0; }
	Vector3D GetProbePosition(U32 x, U32 y, U32 z) const;
	// Of probe x + CountX * (y + CountY * z), the order the probe textures and shaders number them in
	Vector3D GetProbePosition(U32 probe) const;
	// The probes inside box (world space, sides included); empty if there are none, e.g. box is empty or misses the grid
	ProbeRange GetProbesWithin(const BoundingBox& box) const;
};
