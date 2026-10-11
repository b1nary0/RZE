#include "Common/VertexResources.hlsli"

struct VS_IN
{
	float3 Position   : POSITION;
	float4 Colour     : COLOUR;
	float3 FadeCentre : FADECENTRE;
	float2 FadeRange  : FADERANGE;
};

struct VS_OUT
{
	float4 Position   : SV_POSITION;
	float4 Colour     : COLOUR;
	float3 WorldPos   : WORLDPOS;
	float3 FadeCentre : FADECENTRE;
	float2 FadeRange  : FADERANGE;
};

VS_OUT VSMain(VS_IN input)
{
	VS_OUT output;

	const float4 worldPos = mul(World, float4(input.Position, 1.0f));
	output.Position = mul(ViewProjection, worldPos);
	output.Colour = input.Colour;
	output.WorldPos = worldPos.xyz;
	output.FadeCentre = input.FadeCentre;
	output.FadeRange = input.FadeRange;

	return output;
}
