// Probe capture (ProbeCapture): the surface a probe sees through each texel, packed (PackProbeSurface), for relighting
// later with whatever the lights are then. Drawn with no culling, so back faces show and count.
#include "Common/Lighting.hlsli"
#include "Common/ProbeCaptureTypes.hlsli"
#include "Common/Probes.hlsli"

uint2 PSMain(CaptureVertexToPixel input, bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{
	// Cutouts let the probe see through, as they let light through
	ApplyOpacityMask(input.UV);

	ProbeSurface surface;
	surface.IsHit = true;
	surface.IsBackFace = !isFrontFace;
	surface.Albedo = GetAlbedo(input.UV);
	surface.Normal = normalize(input.Normal);
	surface.Distance = length(input.WorldPos - input.ProbePosition);
	return PackProbeSurface(surface);
}
