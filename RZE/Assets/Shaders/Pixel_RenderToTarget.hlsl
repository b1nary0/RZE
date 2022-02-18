struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 UVCoords : UV;
};

Texture2D target : register(t0);
SamplerState samplerState : register(s0);

float4 PSMain(PS_IN input) : SV_TARGET
{
	return target.Sample(samplerState, input.UVCoords);
}