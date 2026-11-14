// Object ID pass (PickRenderStage): writes the object's pick ID so the editor can read back what's under the cursor.
// Cutout pixels are clipped as in the shadow pass, so clicks go through them; so are a blended mask's faint ones.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

// input.ID: the draw's object's 64-bit pick ID as (low half, high half) (IDMeshBatches)
uint2 PSMain(IDVertexToPixel input) : SV_Target
{
	clip(ApplyOpacityMask(input.UV) - AlphaCutoff);
	return input.ID;
}
