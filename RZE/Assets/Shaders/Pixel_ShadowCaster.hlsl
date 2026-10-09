// Shadow map pass (ShadowRenderStage): depth only, no colour output.
// Only does work for cutout materials, so leaves, chains, etc. cast cut-out shadows.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

void PSMain(VertexToPixel input)
{
	ApplyOpacityMask(input.UV);
}
