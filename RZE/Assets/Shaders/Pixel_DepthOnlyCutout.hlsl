// Depth-only passes (DepthOnlyMeshBatches: the depth prepass, the shadow maps): no colour output.
// Bound only for cutout materials, so their cut-out parts neither hide what's behind them nor cast shadows (leaves,
// chains); opaque draws run no pixel shader.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

void PSMain(DepthVertexToPixel input)
{
	ApplyOpacityMask(input.UV);
}
