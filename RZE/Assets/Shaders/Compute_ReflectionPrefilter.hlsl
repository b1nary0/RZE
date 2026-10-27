// Reflection probe prefiltering (ReflectionProbeStage): one mip of the listed probes' prefiltered cubes from their
// radiance cubes. Each mip is what a glossier or rougher surface reflects: the radiance averaged over the Blinn-Phong
// lobe of the shininess that mip stands for (as Phong, a quarter of the exponent), so the forward pass picks a mip by
// gloss (ReflectionEnvironment, Common/ReflectionProbes.hlsli). Mip 0 is a mirror: the radiance itself.
//
// Importance-sampled around each texel's direction, each sample read from the radiance cube's mip whose texels are
// about the sample's share of the lobe (filtered importance sampling), so few samples still blur smoothly.
#include "Common/LightResources.hlsli"
#include "Common/Probes.hlsli"

// Mirrors ReflectionProbeStage::PrefilterParamsLayout
cbuffer PrefilterParams : register(b0)
{
	uint  Mip;
	uint  MipSize;       // Texels across a face at this mip
	float PhongExponent; // The lobe's sharpness at this mip
	uint  _PrefilterPad;
};

TextureCubeArray<float4> Radiance : register(t0); // With mips (GenerateMips)

RWTexture2DArray<float4> OutPrefiltered : register(u0);

#define PREFILTER_SAMPLES 64

// The i-th of count points spread evenly over the unit square (Hammersley)
float2 Hammersley(uint i, uint count)
{
	return float2((i + 0.5f) / count, reversebits(i) * 2.3283064365386963e-10f);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	const uint2 texel = dispatchThreadID.xy;
	const uint listIndex = dispatchThreadID.z / PROBE_FACE_COUNT;
	const uint face = dispatchThreadID.z % PROBE_FACE_COUNT;
	if (any(texel >= MipSize) || listIndex >= ProbeListCount)
	{
		return;
	}

	const uint slot = GetListedProbe(listIndex);
	const float3 N = CubeTexelDirection(face, texel, MipSize);

	float3 colour = 0.0f;
	if (Mip == 0)
	{
		colour = Radiance.SampleLevel(LinearClampSampler, float4(ToCubeSampleDirection(N), slot), 0.0f).rgb;
	}
	else
	{
		const float3 tangent = normalize(cross(abs(N.y) < 0.99f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f), N));
		const float3 bitangent = cross(N, tangent);
		// A face texel's share of the sphere at mip 0 (FaceSize is the probe's capture size)
		const float texelSolidAngle = 4.0f * 3.14159265f / (PROBE_FACE_COUNT * FaceSize * FaceSize);

		for (uint sampleIndex = 0; sampleIndex < PREFILTER_SAMPLES; ++sampleIndex)
		{
			// The Phong lobe cos^n around N: its pdf is (n + 1) / 2pi cos^n
			const float2 xi = Hammersley(sampleIndex, PREFILTER_SAMPLES);
			const float cosTheta = pow(xi.x, 1.0f / (PhongExponent + 1.0f));
			const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
			const float phi = 2.0f * 3.14159265f * xi.y;
			const float3 L = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + N * cosTheta;

			const float pdf = (PhongExponent + 1.0f) / (2.0f * 3.14159265f) * pow(cosTheta, PhongExponent);
			const float sampleSolidAngle = 1.0f / (PREFILTER_SAMPLES * max(pdf, 1e-6f));
			const float lod = max(0.5f * log2(sampleSolidAngle / texelSolidAngle) + 1.0f, 0.0f);

			colour += Radiance.SampleLevel(LinearClampSampler, float4(ToCubeSampleDirection(L), slot), lod).rgb;
		}
		colour /= PREFILTER_SAMPLES;
	}

	OutPrefiltered[uint3(texel, slot * PROBE_FACE_COUNT + face)] = float4(colour, 1.0f);
}
