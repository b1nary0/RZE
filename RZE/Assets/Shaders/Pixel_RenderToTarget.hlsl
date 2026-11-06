#include "Common/FullScreenQuad.hlsli"

Texture2D SourceTexture : register(t0);
SamplerState LinearSampler : register(s0);

// Straight copy: the forward pass already tonemaps and gamma-encodes
float4 PSMain(PS_IN input) : SV_TARGET
{
	return SourceTexture.Sample(LinearSampler, input.UV);
}
