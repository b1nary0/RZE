// Irradiance probe visibility (IrradianceProbeStage), right after a batch is captured: for each direction around the
// probe, the mean and mean squared distance to what it sees, as an octahedral map (Common/IrradianceVolume.hlsli), so
// sampling can tell when a surface is hidden from a probe. A thread group per listed probe (ProbeList, Common/Probes.hlsli),
// a thread per map texel.
//
// A probe that sees mostly back faces is inside geometry, and one that sees no surface in the cells around it has
// nothing to light (the surfaces there are the only ones that read it): either way its map is all 0, which marks it
// inactive. Whether it's active is also written for the CPU to read back, so it isn't relit
// (IrradianceProbeScheduler::SetActive), as DDGI classifies its probes.
#include "Common/IrradianceVolume.hlsli"
#include "Common/Probes.hlsli"

Texture2D<uint2> CaptureAtlas : register(t13);

RWTexture2D<float2> OutVisibility : register(u0);
// For each listed probe, in list order: 1 if it's active, 0 if not
RWTexture2D<uint> OutActive : register(u1);

// How narrow each map texel's average is around its direction: DDGI's depth sharpness
static const float DistanceSharpness = 50.0f;
// More than this share of what a probe sees being back faces makes it inactive, as DDGI
static const float InactiveBackFaceShare = 0.25f;

groupshared float2 g_moments[VISIBILITY_SIZE][VISIBILITY_SIZE];
groupshared uint g_backFaces;
groupshared uint g_hasSurfaceNear;

[numthreads(VISIBILITY_SIZE, VISIBILITY_SIZE, 1)]
void CSMain(uint3 groupID : SV_GroupID, uint2 threadPosition : SV_GroupThreadID, uint threadIndex : SV_GroupIndex)
{
	const uint probe = GetListedProbe(groupID.x);
	if (threadIndex == 0)
	{
		g_backFaces = 0;
		g_hasSurfaceNear = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	const float3 texelDirection = OctahedralDecode((float2(threadPosition) + 0.5f) / VISIBILITY_SIZE * 2.0f - 1.0f);

	const uint texelsPerFace = FaceSize * FaceSize;
	const uint texelCount = texelsPerFace * PROBE_FACE_COUNT;
	const uint2 atlasCorner = uint2((probe % AtlasProbesPerRow) * FaceSize * PROBE_FACE_COUNT, (probe / AtlasProbesPerRow) * FaceSize);

	// Within this of the probe along each axis, a surface is in the cells around it, or reads it through its normal bias
	const float nearReach = VolumeSpacing + VolumeNormalBias;

	float2 moments = 0.0f;
	float weightSum = 0.0f;
	uint backFaces = 0;
	bool hasSurfaceNear = false;
	for (uint texelIndex = 0; texelIndex < texelCount; ++texelIndex)
	{
		const uint face = texelIndex / texelsPerFace;
		const uint withinFace = texelIndex % texelsPerFace;
		const uint2 texel = uint2(withinFace % FaceSize, withinFace / FaceSize);
		const ProbeSurface surface = UnpackProbeSurface(CaptureAtlas.Load(int3(atlasCorner + uint2(face * FaceSize, 0) + texel, 0)));
		const float3 direction = ProbeTexelDirection(face, texel, FaceSize);

		// Each thread counts its share of the probe's back faces, and looks for a surface near it
		if (texelIndex % (VISIBILITY_SIZE * VISIBILITY_SIZE) == threadIndex && surface.IsHit)
		{
			backFaces += surface.IsBackFace ? 1 : 0;
			hasSurfaceNear = hasSurfaceNear || all(abs(direction * surface.Distance) <= nearReach);
		}

		const float weight = pow(saturate(dot(texelDirection, direction)), DistanceSharpness);
		if (weight > 0.0f)
		{
			const float distance = surface.IsHit ? min(surface.Distance, VisibilityMaxDistance) : VisibilityMaxDistance;
			moments += weight * float2(distance, distance * distance);
			weightSum += weight;
		}
	}
	moments /= max(weightSum, 1e-5f);

	InterlockedAdd(g_backFaces, backFaces);
	if (hasSurfaceNear)
	{
		g_hasSurfaceNear = 1;
	}
	GroupMemoryBarrierWithGroupSync();

	const bool isActive = float(g_backFaces) <= float(texelCount) * InactiveBackFaceShare && g_hasSurfaceNear != 0;
	if (!isActive)
	{
		moments = 0.0f;
	}
	if (threadIndex == 0)
	{
		OutActive[uint2(groupID.x, 0)] = isActive ? 1u : 0u;
	}

	const int2 mapCorner = ProbeVisibilityCorner(probe);
	OutVisibility[mapCorner + 1 + int2(threadPosition)] = moments;
	g_moments[threadPosition.x][threadPosition.y] = moments;
	GroupMemoryBarrierWithGroupSync();

	// The border, for bilinear filtering across the map's edges. The square's edges meet themselves mirrored (the same
	// direction at v and -v on a left or right edge, at u and -u on the top or bottom), and its corners are all -Z.
	const uint last = VISIBILITY_SIZE - 1;
	const uint2 p = threadPosition;
	if (p.x == 0)
	{
		OutVisibility[mapCorner + int2(0, 1 + p.y)] = g_moments[0][last - p.y];
	}
	if (p.x == last)
	{
		OutVisibility[mapCorner + int2(VISIBILITY_SIZE + 1, 1 + p.y)] = g_moments[last][last - p.y];
	}
	if (p.y == 0)
	{
		OutVisibility[mapCorner + int2(1 + p.x, 0)] = g_moments[last - p.x][0];
	}
	if (p.y == last)
	{
		OutVisibility[mapCorner + int2(1 + p.x, VISIBILITY_SIZE + 1)] = g_moments[last - p.x][last];
	}
	if (threadIndex == 0)
	{
		OutVisibility[mapCorner + int2(0, 0)] = g_moments[last][last];
		OutVisibility[mapCorner + int2(VISIBILITY_SIZE + 1, 0)] = g_moments[0][last];
		OutVisibility[mapCorner + int2(0, VISIBILITY_SIZE + 1)] = g_moments[last][0];
		OutVisibility[mapCorner + int2(VISIBILITY_SIZE + 1, VISIBILITY_SIZE + 1)] = g_moments[0][0];
	}
}
