struct VS_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
	float3 Tangent : TANGENT;
};

struct VS_OUT
{
	float4 FragPos : SV_POSITION;
};

cbuffer LightSpaceBuffer : register(b0)
{
	float4x4 LightSpaceMatrix;
};

cbuffer MVPBuffer : register(b0, space1)
{
	matrix ModelView;
	matrix ModelViewInv;
	matrix CameraViewProjection;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	float4 fragPos = mul(ModelView, float4(input.Position, 1.0f));
	fragPos = mul(LightSpaceMatrix, fragPos);
	output.FragPos = fragPos;
	
    return output;
}