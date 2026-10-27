// Irradiance probe capture (ProbeCapture): the surface a probe sees through each texel, packed (PackProbeSurface), for
// relighting later with whatever the lights are then. Drawn with no culling, so back faces show and count.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/Probes.hlsli"

// Mirrors ProbeCapture::ParamsLayout
cbuffer ProbeCaptureBuffer : register(b5)
{
	float3 ProbePosition;
	float  _Pad;
};

uint2 PSMain(VertexToPixel input, bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{
	// Cutouts let the probe see through, as they let light through
	ApplyOpacityMask(input.UV);

	ProbeSurface surface;
	surface.IsHit = true;
	surface.IsBackFace = !isFrontFace;
	surface.Albedo = GetAlbedo(input.UV);
	surface.Normal = normalize(input.Normal);
	surface.Distance = length(input.WorldPos - ProbePosition);
	return PackProbeSurface(surface);
}
