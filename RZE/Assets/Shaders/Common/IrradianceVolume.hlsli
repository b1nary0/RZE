#ifndef IRRADIANCE_VOLUME_HLSLI
#define IRRADIANCE_VOLUME_HLSLI

// The scene's irradiance probes (IrradianceProbeStage): diffuse light at points on a grid, as order-1 spherical
// harmonics, one texture per colour channel. Read by the forward pass for ambient light, and by the probes' own
// relighting for the light bouncing between surfaces. Mirrors Utils/Math/SphericalHarmonics.h.

// Mirrors GraphicsLayouts::IrradianceVolumeLayout (Graphics/GraphicsLayouts.h)
cbuffer IrradianceVolume : register(b5)
{
	float3 VolumeOrigin;
	float  VolumeSpacing;
	uint3  VolumeCounts;
	uint   ValidProbeCount;
	float  VolumeNormalBias;
	float3 _VolumePad;
};

// A probe's coefficients (Y00, Y1-1, Y10, Y11) for one channel at texel (x, y) of slice z
Texture2DArray<float4> ProbeSHRed   : register(t8);
Texture2DArray<float4> ProbeSHGreen : register(t9);
Texture2DArray<float4> ProbeSHBlue  : register(t10);

int3 ProbeCoordinates(uint probe)
{
	return int3(probe % VolumeCounts.x, (probe / VolumeCounts.x) % VolumeCounts.y, probe / (VolumeCounts.x * VolumeCounts.y));
}

// What a white diffuse surface facing normal reflects at one probe (irradiance / pi), as
// SphericalHarmonicsL1::EvaluateIrradiance
float3 EvaluateProbeIrradiance(int3 probe, float3 normal)
{
	// The basis, times the clamped cosine's convolution over pi: 1 for band 0, 2/3 for band 1
	const float4 basis = float4(0.282094792f, 0.488602512f * normal.y, 0.488602512f * normal.z, 0.488602512f * normal.x)
		* float4(1.0f, 2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f);
	const int4 location = int4(probe, 0);
	// L1 can ring below 0 facing away from a bright direction
	return max(float3(dot(ProbeSHRed.Load(location), basis), dot(ProbeSHGreen.Load(location), basis), dot(ProbeSHBlue.Load(location), basis)), 0.0f);
}

// The diffuse light reaching a surface at worldPos facing normal (irradiance / pi), blended from the 8 probes around it:
// by distance (trilinear), and less from probes behind the surface, which see its other side. False with no probes to
// read there, e.g. before they're lit.
bool SampleProbeIrradiance(float3 worldPos, float3 normal, out float3 irradiance)
{
	irradiance = 0.0f;
	if (ValidProbeCount == 0)
	{
		return false;
	}

	const float3 samplePos = worldPos + normal * VolumeNormalBias;
	const float3 gridPos = clamp((samplePos - VolumeOrigin) / VolumeSpacing, 0.0f, float3(VolumeCounts - 1));
	// A grid has at least 2 probes along each axis
	const int3 base = min(int3(floor(gridPos)), int3(VolumeCounts) - 2);
	const float3 alpha = gridPos - base;

	float3 sum = 0.0f;
	float weightSum = 0.0f;
	[unroll]
	for (int corner = 0; corner < 8; ++corner)
	{
		const int3 offset = int3(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
		const int3 probe = base + offset;
		const uint index = probe.x + VolumeCounts.x * (probe.y + VolumeCounts.y * probe.z);
		if (index >= ValidProbeCount)
		{
			continue;
		}

		const float3 trilinear = lerp(1.0f - alpha, alpha, float3(offset));
		float weight = trilinear.x * trilinear.y * trilinear.z;

		// Smoothly less from probes behind the surface, as DDGI does, but never none, so a surface with probes only
		// behind it still gets some light
		const float3 toProbe = VolumeOrigin + float3(probe) * VolumeSpacing - worldPos;
		const float wrap = (dot(toProbe * rsqrt(max(dot(toProbe, toProbe), 1e-8f)), normal) + 1.0f) * 0.5f;
		weight *= wrap * wrap + 0.2f;

		sum += weight * EvaluateProbeIrradiance(probe, normal);
		weightSum += weight;
	}

	if (weightSum < 1e-4f)
	{
		return false;
	}

	irradiance = sum / weightSum;
	return true;
}

#endif // IRRADIANCE_VOLUME_HLSLI
