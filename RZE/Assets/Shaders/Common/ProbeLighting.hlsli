#ifndef PROBE_LIGHTING_HLSLI
#define PROBE_LIGHTING_HLSLI

// How a surface a probe captured is lit: shared by the irradiance probes' relighting (Compute_ProbeRelight.hlsl) and the
// reflection probes' (Compute_ReflectionRelight.hlsl), so both see the same light.

#include "Common/LightResources.hlsli"
#include "Common/IrradianceVolume.hlsli"
#include "Common/Probes.hlsli"

// The light leaving a captured surface towards the probe (per unit albedo conventions as the forward pass: no 1/pi)
float3 SurfaceRadiance(ProbeSurface surface, float3 worldPos)
{
	float3 direct = 0.0f;
	for (uint lightIndex = 0; lightIndex < LightCount; ++lightIndex)
	{
		Light light = Lights[lightIndex];

		float3 L;
		const float attenuation = LightAttenuation(light, worldPos, L);
		if (attenuation <= 0.0f)
		{
			continue;
		}

		direct += saturate(dot(surface.Normal, L)) * LightRadiance(light) * attenuation * LightShadow(light, worldPos, surface.Normal);
	}

	// Last frame's probes, or the sky and ground as the forward pass falls back to
	float3 indirect;
	if (!SampleProbeIrradiance(worldPos, surface.Normal, indirect))
	{
		indirect = SkyRadiance(surface.Normal);
	}

	return surface.Albedo * (direct + indirect);
}

#endif // PROBE_LIGHTING_HLSLI
