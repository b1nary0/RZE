// Irradiance probe relighting (IrradianceProbeStage): lights what each probe captured (ProbeCapture) with the scene's
// lights as they are now, and projects the light arriving from every direction onto spherical harmonics. A thread group
// per probe; each thread takes every 64th texel of it, then the group adds up.
//
// A captured surface is lit by the lights, past their shadows, and by the probes' light from last frame (bound as the
// volume), so the light bounces once more each frame. The sky lights what sees nothing; back faces, inside or under
// something, give nothing.
#include "Common/LightResources.hlsli"
#include "Common/IrradianceVolume.hlsli"
#include "Common/Probes.hlsli"

// Mirrors IrradianceProbeStage::RelightParamsLayout
cbuffer RelightParams : register(b6)
{
	uint FirstProbe;        // The batch is this and the probes after it, wrapping round at RelightRange
	uint RelightRange;
	uint FaceSize;          // Texels across a capture face
	uint AtlasProbesPerRow;
};

Texture2D<uint2> CaptureAtlas : register(t11);

RWTexture2DArray<float4> OutSHRed   : register(u0);
RWTexture2DArray<float4> OutSHGreen : register(u1);
RWTexture2DArray<float4> OutSHBlue  : register(u2);

#define RELIGHT_THREADS 64

groupshared float4 g_red[RELIGHT_THREADS];
groupshared float4 g_green[RELIGHT_THREADS];
groupshared float4 g_blue[RELIGHT_THREADS];
groupshared float  g_solidAngle[RELIGHT_THREADS];

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

[numthreads(RELIGHT_THREADS, 1, 1)]
void CSMain(uint3 groupID : SV_GroupID, uint threadIndex : SV_GroupIndex)
{
	const uint probe = (FirstProbe + groupID.x) % RelightRange;
	const int3 coordinates = ProbeCoordinates(probe);
	const float3 probePosition = VolumeOrigin + float3(coordinates) * VolumeSpacing;

	const uint texelsPerFace = FaceSize * FaceSize;
	const uint2 atlasCorner = uint2((probe % AtlasProbesPerRow) * FaceSize * PROBE_FACE_COUNT, (probe / AtlasProbesPerRow) * FaceSize);

	float4 red = 0.0f;
	float4 green = 0.0f;
	float4 blue = 0.0f;
	float solidAngleSum = 0.0f;
	for (uint texelIndex = threadIndex; texelIndex < texelsPerFace * PROBE_FACE_COUNT; texelIndex += RELIGHT_THREADS)
	{
		const uint face = texelIndex / texelsPerFace;
		const uint withinFace = texelIndex % texelsPerFace;
		const uint2 texel = uint2(withinFace % FaceSize, withinFace / FaceSize);
		const float3 direction = ProbeTexelDirection(face, texel, FaceSize);

		const ProbeSurface surface = UnpackProbeSurface(CaptureAtlas.Load(int3(atlasCorner + uint2(face * FaceSize, 0) + texel, 0)));
		float3 radiance = 0.0f;
		if (!surface.IsHit)
		{
			radiance = SkyRadiance(direction);
		}
		else if (!surface.IsBackFace)
		{
			radiance = SurfaceRadiance(surface, probePosition + direction * surface.Distance);
		}

		// The texel's share of the sphere: a cube face texel shrinks towards the face's corners
		const float2 ndc = float2(2.0f * (texel.x + 0.5f) / FaceSize - 1.0f, 1.0f - 2.0f * (texel.y + 0.5f) / FaceSize);
		const float solidAngle = 4.0f / (FaceSize * FaceSize) * pow(1.0f + dot(ndc, ndc), -1.5f);

		// As SphericalHarmonicsL1::AddSample
		const float4 basis = float4(0.282094792f, 0.488602512f * direction.y, 0.488602512f * direction.z, 0.488602512f * direction.x) * solidAngle;
		red += radiance.r * basis;
		green += radiance.g * basis;
		blue += radiance.b * basis;
		solidAngleSum += solidAngle;
	}

	g_red[threadIndex] = red;
	g_green[threadIndex] = green;
	g_blue[threadIndex] = blue;
	g_solidAngle[threadIndex] = solidAngleSum;
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint stride = RELIGHT_THREADS / 2; stride > 0; stride /= 2)
	{
		if (threadIndex < stride)
		{
			g_red[threadIndex] += g_red[threadIndex + stride];
			g_green[threadIndex] += g_green[threadIndex + stride];
			g_blue[threadIndex] += g_blue[threadIndex + stride];
			g_solidAngle[threadIndex] += g_solidAngle[threadIndex + stride];
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (threadIndex == 0)
	{
		// The texels' solid angles are per texel centre, so add up to a little off the sphere's 4 pi
		const float normalisation = 4.0f * 3.14159265f / g_solidAngle[0];
		OutSHRed[coordinates] = g_red[0] * normalisation;
		OutSHGreen[coordinates] = g_green[0] * normalisation;
		OutSHBlue[coordinates] = g_blue[0] * normalisation;
	}
}
