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
	matrix ModelView;
	matrix ModelViewInv;
	matrix CameraViewProjection;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	float3 fragPos = mul(ModelView, float4(input.Position, 1.0f)).xyz;
	
	output.Position = mul(CameraViewProjection, float4(fragPos, 1.0f));
	output.Normal = mul(transpose(ModelViewInv), float4(input.Normal, 1.0f)).xyz;
	output.Tangent = mul(ModelView, float4(input.Tangent, 1.0f)).xyz;
	output.Color = input.Color;
	output.FragPos = fragPos;
	
    return output;
}