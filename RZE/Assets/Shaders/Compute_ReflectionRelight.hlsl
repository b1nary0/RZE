// Reflection probe relighting (ReflectionProbeStage): lights what each listed probe captured (in CubeMap's face layout,
// ProbeCapture::EProbeFaces::CubeMap) with the scene's lights and irradiance probes, into its six slices of the
// radiance cube array. A thread per texel; z picks the listed probe and its face.
#include "Common/ProbeLighting.hlsli"

// Mirrors ReflectionProbeStage::RelightParamsLayout: each probe slot's position
cbuffer ReflectionRelightParams : register(b0)
{
	float4 SlotPositions[16];
};

// A probe a row, its six faces side by side (ProbeList's AtlasProbesPerRow is 1)
Texture2D<uint2> CaptureAtlas : register(t13);

// Slot s's faces are slices 6s to 6s + 5
RWTexture2DArray<float4> OutRadiance : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	const uint2 texel = dispatchThreadID.xy;
	const uint listIndex = dispatchThreadID.z / PROBE_FACE_COUNT;
	const uint face = dispatchThreadID.z % PROBE_FACE_COUNT;
	if (any(texel >= FaceSize) || listIndex >= ProbeListCount)
	{
		return;
	}

	const uint slot = GetListedProbe(listIndex);
	const float3 direction = CubeTexelDirection(face, texel, FaceSize);
	const ProbeSurface surface = UnpackProbeSurface(CaptureAtlas.Load(int3(face * FaceSize + texel.x, slot * FaceSize + texel.y, 0)));

	float3 radiance = 0.0f;
	if (!surface.IsHit)
	{
		radiance = SkyRadiance(direction);
	}
	else if (!surface.IsBackFace)
	{
		radiance = SurfaceRadiance(surface, SlotPositions[slot].xyz + direction * surface.Distance);
	}

	OutRadiance[uint3(texel, slot * PROBE_FACE_COUNT + face)] = float4(radiance, 1.0f);
}
