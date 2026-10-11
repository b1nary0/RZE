// Object ID pass (PickRenderStage): writes the object's pick ID so the editor can read back what's under the cursor.
// Cutout pixels are clipped as in the shadow pass, so clicks go through them.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

// Mirrors PickRenderStage::PickBufferLayout
cbuffer PickBuffer : register(b0)
{
	uint PickID;
};

uint PSMain(VertexToPixel input) : SV_Target
{
	ApplyOpacityMask(input.UV);
	return PickID;
}
