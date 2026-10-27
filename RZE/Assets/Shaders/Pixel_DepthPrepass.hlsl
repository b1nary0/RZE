// Depth prepass (DepthPrepassStage): depth only, no colour output.
// Bound only for cutout materials, so their cut-out parts don't hide what's behind them; opaque draws run no pixel shader.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

void PSMain(VertexToPixel input)
{
	ApplyOpacityMask(input.UV);
}
