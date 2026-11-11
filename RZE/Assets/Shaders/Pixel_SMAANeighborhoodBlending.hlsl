// SMAA's third pass: blends each pixel with its neighbours across their edges by the weights, into the view's target
#include "Common/SMAA/SMAA.hlsli"

Texture2D ColourTexture : register(t0);
Texture2D BlendTexture : register(t1);

float4 PSMain(PS_IN input) : SV_TARGET
{
	const float2 texcoord = SMAATexcoord(input);
	float4 offset;
	SMAANeighborhoodBlendingVS(texcoord, offset);
	return SMAANeighborhoodBlendingPS(texcoord, offset, ColourTexture, BlendTexture);
}
