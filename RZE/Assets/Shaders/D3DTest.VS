struct VS_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
};

cbuffer MVPBuffer : register(b0)
{
	matrix ModelView;
	matrix MVP;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	output.Position = mul(MVP, float4(input.Position, 1.0f));
	output.Normal = normalize(mul(transpose(ModelView), input.Normal));
	output.Color = input.Color;
	
    return output;
}