#include "Common/VertexResources.hlsli"

struct VS_IN
{
	float3 Position : POSITION;
	float3 Colour   : COLOUR;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float3 Colour   : COLOUR;
};

VS_OUT VSMain(VS_IN input)
{
	VS_OUT output;

	output.Position = mul(ViewProjection, mul(World, float4(input.Position, 1.0f)));
	output.Colour = input.Colour;

	return output;
}
