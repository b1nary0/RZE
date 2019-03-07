struct VS_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
	float3 Tangent : TANGENT;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
};

cbuffer MVPBuffer : register(b0)
{
	matrix ModelViewInv;
	matrix MVP;
	matrix ModelView;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	float3 fragPos = mul(ModelView, float4(input.Position, 1.0f)).xyz;
	
	output.Position = mul(MVP, float4(input.Position, 1.0f));
	output.Normal = mul(transpose(ModelViewInv), input.Normal).xyz;
	output.Tangent = input.Tangent;
	output.Color = input.Color;
	output.FragPos = fragPos;
	
    return output;
}