#ifndef REFLECTION_PROBES_HLSLI
#define REFLECTION_PROBES_HLSLI

// The scene's reflection probes (ReflectionProbeStage), as the forward pass reads them: each probe's box and its
// prefiltered cube, mip by gloss (Compute_ReflectionPrefilter.hlsl).

#include "Common/LightResources.hlsli"
#include "Common/Probes.hlsli"

// Mirrors GraphicsLayouts::k_maxReflectionProbes (Graphics/GraphicsLayouts.h)
#define MAX_REFLECTION_PROBES 16

// Mirrors GraphicsLayouts::ReflectionProbeLayout
struct ReflectionProbe
{
	float3 BoxMin;
	float  BlendDistance; // How far inside the box's sides its reflection fades in
	float3 BoxMax;
	float  Slot;          // Its cube in ReflectionCubes
	float3 Position;      // Where it captured from
	float  _Pad;
};

// Mirrors GraphicsLayouts::ReflectionProbeBufferLayout
cbuffer ReflectionProbes : register(b7)
{
	uint ReflectionProbeCount;
	uint3 _ReflectionProbesPad;
	ReflectionProbe ReflectionProbeList[MAX_REFLECTION_PROBES];
};

// A cube per probe slot, REFLECTION_MIP_COUNT mips; sample with ToCubeSampleDirection
TextureCubeArray<float4> ReflectionCubes : register(t12);

// What a surface at worldPos reflects along R (unit length) at gloss (0 rough, 1 mirror): the reflection probes whose
// boxes it's in, blended by how far inside each it is, each projected onto its box as if the box were the room. The
// rest of the weight is fallback's. outCoverage is how much the probes cover, 0 to 1.
float3 ReflectionEnvironment(float3 worldPos, float3 R, float gloss, float3 fallback, out float outCoverage)
{
	float3 sum = 0.0f;
	float weightSum = 0.0f;
	for (uint probeIndex = 0; probeIndex < ReflectionProbeCount; ++probeIndex)
	{
		const ReflectionProbe probe = ReflectionProbeList[probeIndex];
		if (any(worldPos < probe.BoxMin) || any(worldPos > probe.BoxMax))
		{
			continue;
		}

		// Fades in over the blend distance inside the box's nearest side
		const float3 inside = min(worldPos - probe.BoxMin, probe.BoxMax - worldPos);
		const float toNearestSide = min(inside.x, min(inside.y, inside.z));
		const float weight = probe.BlendDistance > 0.0f ? saturate(toNearestSide / probe.BlendDistance) : 1.0f;

		// Where the reflected ray leaves the box, seen from where the probe captured: the box stands in for the room
		const float3 safeR = sign(R) * max(abs(R), 1e-5f) + (R == 0.0f) * 1e-5f;
		const float3 toMax = (probe.BoxMax - worldPos) / safeR;
		const float3 toMin = (probe.BoxMin - worldPos) / safeR;
		const float3 exits = max(toMax, toMin);
		const float exitDistance = min(exits.x, min(exits.y, exits.z));
		const float3 direction = worldPos + R * exitDistance - probe.Position;

		const float mip = (1.0f - gloss) * (REFLECTION_MIP_COUNT - 1);
		sum += weight * ReflectionCubes.SampleLevel(LinearClampSampler, float4(ToCubeSampleDirection(direction), probe.Slot), mip).rgb;
		weightSum += weight;
	}

	outCoverage = saturate(weightSum);
	if (weightSum >= 1.0f)
	{
		return sum / weightSum;
	}
	return sum + (1.0f - weightSum) * fallback;
}

#endif // REFLECTION_PROBES_HLSLI
