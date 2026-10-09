#include "Common/VertexResources.hlsli"

// Line vertices are currently uploaded as MeshVertex, so COLOUR actually reads the normal slot.
// Use a constant colour until line vertices carry real colour data.
static const float3 LineColour = float3(1.0f, 0.0f, 0.0f);

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
	output.Colour = LineColour;

	return output;
}
