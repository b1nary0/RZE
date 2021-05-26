struct VS_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
	float3 Tangent : TANGENT;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
};

cbuffer ViewProjectionBuf : register(b0)
{
	matrix ViewProj;
};

cbuffer ModelMatBuf : register(b1)
{
	matrix ModelView;
	matrix InvModelView;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	matrix mvp = mul(ViewProj, ModelView);
	
	output.Position = mul(mvp, float4(input.Position, 1.0f));
	output.FragPos = mul(ModelView, float4(input.Position, 1.0f)).xyz;;
	output.Normal = normalize(mul(ModelView, input.Normal));
	output.Tangent = mul(InvModelView, float4(input.Tangent, 1.0f)).xyz;
	output.UVCoords = input.UVCoords;
	
    return output;
}