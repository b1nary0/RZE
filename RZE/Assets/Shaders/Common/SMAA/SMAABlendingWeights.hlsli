// SMAA's second pass: how much each edge pixel blends with its neighbour across the edge, from the edge's shape (its
// length and where its ends turn) looked up in the area texture. Shared by Pixel_SMAABlendingWeights and
// Pixel_SMAABlendingWeightsNoDiagonals, which differ only in SMAA_DISABLE_DIAG/CORNER_DETECTION.
#include "Common/SMAA/SMAA.hlsli"

Texture2D EdgesTexture : register(t0);
Texture2D AreaTexture : register(t1);
Texture2D SearchTexture : register(t2);

float4 PSMain(PS_IN input) : SV_TARGET
{
	const float2 texcoord = SMAATexcoord(input);
	// SMAA's vertex-shader half, here per pixel: no new vertex shader, and nothing extra through the GL translator
	float2 pixcoord;
	float4 offset[3];
	SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
	// SMAA 1x: no subsamples
	return SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, EdgesTexture, AreaTexture, SearchTexture, float4(0.0f, 0.0f, 0.0f, 0.0f));
}
