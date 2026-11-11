// SMAA's first pass: where the tonemapped image's luma steps sharply to the left (r) or top (g) neighbour. Pixels
// without an edge are discarded, so AntiAliasingStage clears the edges texture first.
#include "Common/SMAA/SMAA.hlsli"

// The tonemapped, sRGB-encoded image, read as is, as SMAA's luma edges expect
Texture2D ColourTexture : register(t0);

float4 PSMain(PS_IN input) : SV_TARGET
{
	const float2 texcoord = SMAATexcoord(input);
	float4 offset[3];
	SMAAEdgeDetectionVS(texcoord, offset);
	return float4(SMAALumaEdgeDetectionPS(texcoord, offset, ColourTexture), 0.0f, 0.0f);
}
