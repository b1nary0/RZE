// Outline mask (OutlineRenderStage): which outline's object is drawn at each pixel, which shape it's in, and whether
// it's in front of the scene there. Cutout pixels are clipped as in the shadow pass, so a leaf's outline follows its
// cut-out shape; so are a blended mask's faint ones.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/Outline.hlsli"

// The scene's depth (DepthPrepassStage), past the forward pass's textures
Texture2D<float> SceneDepth : register(t13);

// The scene's depth is D24S8: depth as a 24-bit unorm, so one step is 1 / (2^24 - 1)
static const float DepthStep = 1.0f / float(0xFFFFFF);

// input.ID: the draw's outline's index + 1, then its shape (OutlineRenderStage::OutlinedObject)
uint PSMain(IDVertexToPixel input) : SV_Target
{
	clip(ApplyOpacityMask(input.UV) - AlphaCutoff);
	const uint outlineID = input.ID.x;
	const uint shapeID = input.ID.y;

	// Placed as the prepass places vertices (Vertex_MeshID), so where this object is the surface the prepass kept, the
	// depths are the same but for the stored one's rounding to 24 bits
	const float sceneDepth = SceneDepth.Load(int3(int2(input.Position.xy), 0));
	const bool isInFront = input.Position.z <= sceneDepth + 2.0f * DepthStep;
	return outlineID | (isInFront ? OutlineInFrontBit : 0u) | (shapeID << OutlineShapeShift);
}
