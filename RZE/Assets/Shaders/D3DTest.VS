struct VS_IN
{
	float3 Position : POSITION;
	float4 Color : NORMAL;
};

struct VS_OUT
{
	float4 Color : NORMAL;
	float4 Position : SV_POSITION;
};

cbuffer MVPBuffer : register(b0)
{
	matrix MVP;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	output.Position = mul(MVP, float4(input.Position, 1.0f));
	output.Color = input.Color;
	
    return output;
}