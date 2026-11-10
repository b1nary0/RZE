// Outline mask (OutlineRenderStage): which outline's object is drawn at each pixel, and whether it's in front of the
// scene there. Cutout pixels are clipped as in the shadow pass, so a leaf's outline follows its cut-out shape; so are a
// blended mask's faint ones.
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"
#include "Common/Outline.hlsli"

// Mirrors OutlineRenderStage::MaskBufferLayout
cbuffer OutlineMaskBuffer : register(b0)
{
	uint OutlineID;
};

// The scene's depth (DepthPrepassStage), past the forward pass's textures
Texture2D<float> SceneDepth : register(t13);

// The scene's depth is D24S8: depth as a 24-bit unorm, so one step is 1 / (2^24 - 1)
static const float DepthStep = 1.0f / float(0xFFFFFF);

uint PSMain(VertexToPixel input) : SV_Target
{
	clip(ApplyOpacityMask(input.UV) - AlphaCutoff);

	// Drawn with the prepass's vertex shader, so where this object is the surface the prepass kept, the depths are the
	// same but for the stored one's rounding to 24 bits
	const float sceneDepth = SceneDepth.Load(int3(int2(input.Position.xy), 0));
	const bool isInFront = input.Position.z <= sceneDepth + 2.0f * DepthStep;
	return OutlineID | (isInFront ? OutlineInFrontBit : 0u);
}
