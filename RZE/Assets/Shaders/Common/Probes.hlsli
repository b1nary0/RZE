#ifndef PROBES_HLSLI
#define PROBES_HLSLI

// What irradiance probes capture and how it's stored, shared by the capture (Pixel_ProbeCapture.hlsl) and the passes that
// read the captures. Mirrors Graphics/ProbeCapture.h and Utils/Math/Octahedral.h.

#include "Common/ColourSpace.hlsli"

// A probe sees through six faces, each a square of texels, side by side in that order (ProbeCapture::GetFaceBasis)
#define PROBE_FACE_COUNT 6

// The direction each face looks along, and its up
static const float3 ProbeFaceForward[PROBE_FACE_COUNT] =
{
	float3(1.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, -1.0f, 0.0f),
	float3(0.0f, 0.0f, 1.0f), float3(0.0f, 0.0f, -1.0f)
};
static const float3 ProbeFaceUp[PROBE_FACE_COUNT] =
{
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.0f, -1.0f), float3(0.0f, 0.0f, 1.0f),
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)
};

// The direction from the probe through the centre of texel (x, y) of a face, from its top-left. Mirrors
// ProbeCapture::GetTexelDirection.
float3 ProbeTexelDirection(uint face, uint2 texel, uint faceSize)
{
	const float3 forward = ProbeFaceForward[face];
	const float3 right = normalize(cross(forward, ProbeFaceUp[face]));
	const float3 up = cross(right, forward);
	const float2 ndc = float2(2.0f * (texel.x + 0.5f) / faceSize - 1.0f, 1.0f - 2.0f * (texel.y + 0.5f) / faceSize);
	return normalize(forward + ndc.x * right + ndc.y * up);
}

// A cube texture's slices, as CubeMap lays them out (CubeMap::GetFaceForward and GetFaceUp, Engine/Src/Graphics/CubeMap.h): +X,
// -X, +Y, -Y, -Z (Forward), +Z (Back). Reflection probes capture in this layout (ProbeCapture::EProbeFaces::CubeMap), so
// their texels are a cube's; sample the cube with ToCubeSampleDirection (Common/LightResources.hlsli).
static const float3 CubeFaceForward[PROBE_FACE_COUNT] =
{
	float3(1.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, -1.0f, 0.0f),
	float3(0.0f, 0.0f, -1.0f), float3(0.0f, 0.0f, 1.0f)
};
static const float3 CubeFaceUp[PROBE_FACE_COUNT] =
{
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.0f, 1.0f), float3(0.0f, 0.0f, -1.0f),
	float3(0.0f, 1.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)
};

// The world direction through the centre of texel (x, y) of a cube slice, as ProbeTexelDirection does for a probe face
float3 CubeTexelDirection(uint face, uint2 texel, uint faceSize)
{
	const float3 forward = CubeFaceForward[face];
	const float3 right = normalize(cross(forward, CubeFaceUp[face]));
	const float3 up = cross(right, forward);
	const float2 ndc = float2(2.0f * (texel.x + 0.5f) / faceSize - 1.0f, 1.0f - 2.0f * (texel.y + 0.5f) / faceSize);
	return normalize(forward + ndc.x * right + ndc.y * up);
}

// Mips of a reflection probe's prefiltered cube: mip 0 is mirror-sharp, each further one a wider lobe (Compute_ReflectionPrefilter)
#define REFLECTION_MIP_COUNT 6

// Mirrors Octahedral::Encode: a unit direction to -1..1 on both axes
float2 OctahedralEncode(float3 direction)
{
	direction /= abs(direction.x) + abs(direction.y) + abs(direction.z);
	if (direction.z >= 0.0f)
	{
		return direction.xy;
	}
	const float2 signs = float2(direction.x >= 0.0f ? 1.0f : -1.0f, direction.y >= 0.0f ? 1.0f : -1.0f);
	return (1.0f - abs(direction.yx)) * signs;
}

// Mirrors Octahedral::Decode
float3 OctahedralDecode(float2 encoded)
{
	float3 direction = float3(encoded, 1.0f - abs(encoded.x) - abs(encoded.y));
	const float fold = max(-direction.z, 0.0f);
	direction.x += direction.x >= 0.0f ? -fold : fold;
	direction.y += direction.y >= 0.0f ? -fold : fold;
	return normalize(direction);
}

// What a probe sees through one texel: the nearest surface there, or nothing (the sky)
struct ProbeSurface
{
	bool   IsHit;
	bool   IsBackFace; // The probe sees the surface's back: it's inside or under something
	float3 Albedo;     // Linear
	float3 Normal;     // World space
	float  Distance;   // Metres from the probe
};

#define PROBE_SURFACE_HIT       1u
#define PROBE_SURFACE_BACK_FACE 2u

// Into an RG32_UINT texel: x is sRGB albedo in 8 bits a channel and the flags above in the top byte; y is the normal
// (octahedral, 8 bits a side) and the distance as a 16-bit float. A texel cleared to 0 is a miss.
uint2 PackProbeSurface(ProbeSurface surface)
{
	const uint3 albedo = (uint3)round(saturate(LinearToSRGB(surface.Albedo)) * 255.0f);
	const uint flags = PROBE_SURFACE_HIT | (surface.IsBackFace ? PROBE_SURFACE_BACK_FACE : 0u);
	const uint2 normal = (uint2)round(saturate(OctahedralEncode(surface.Normal) * 0.5f + 0.5f) * 255.0f);

	uint2 packed;
	packed.x = albedo.r | (albedo.g << 8) | (albedo.b << 16) | (flags << 24);
	packed.y = normal.x | (normal.y << 8) | (f32tof16(surface.Distance) << 16);
	return packed;
}

ProbeSurface UnpackProbeSurface(uint2 packed)
{
	const uint flags = packed.x >> 24;

	ProbeSurface surface;
	surface.IsHit = (flags & PROBE_SURFACE_HIT) != 0;
	surface.IsBackFace = (flags & PROBE_SURFACE_BACK_FACE) != 0;
	surface.Albedo = SRGBToLinear(float3(packed.x & 0xFF, (packed.x >> 8) & 0xFF, (packed.x >> 16) & 0xFF) / 255.0f);
	surface.Normal = OctahedralDecode(float2(packed.y & 0xFF, (packed.y >> 8) & 0xFF) / 255.0f * 2.0f - 1.0f);
	surface.Distance = f16tof32(packed.y >> 16);
	return surface;
}

// The probes a compute pass works on (Compute_ProbeVisibility, Compute_ProbeRelight): a thread group each, in this
// order. Mirrors IrradianceProbeStage::ProbeListLayout.
#define MAX_PROBE_LIST 512
cbuffer ProbeList : register(b6)
{
	uint  ProbeListCount;
	uint  FaceSize;          // Texels across a capture face
	uint  AtlasProbesPerRow; // In the capture atlas
	uint  _ProbeListPad;
	uint4 ProbeListEntries[MAX_PROBE_LIST / 4];
};

uint GetListedProbe(uint index)
{
	return ProbeListEntries[index / 4][index % 4];
}

#endif // PROBES_HLSLI
