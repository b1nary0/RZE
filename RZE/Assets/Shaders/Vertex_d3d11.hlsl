struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Color : UV;
};

cbuffer ViewProjectionBuf : register(b0)
{
	matrix ViewProj;
};

cbuffer ModelMatBuf : register(b1)
{
	matrix ModelMat;
};

VS_OUTPUT VS(float3 inPos : POSITION, float2 inColor : TEXCOORD)
{
    VS_OUTPUT output;

	float4 transformedPos = float4(inPos, 1.0f);

    transformedPos = mul(ModelMat, transformedPos);
    transformedPos = mul(ViewProj, transformedPos);

    output.Pos = transformedPos;
    output.Color = inColor;

    return output;
}

Texture2D diffuse : register(t0);
Texture2D specular : register(t1);
Texture2D bump : register(t2);

SamplerState diffSampleState : register(s0);
SamplerState specSampleState : register(s1);
SamplerState bumpSampleState : register(s2);

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	float4 diffSample = diffuse.Sample(diffSampleState, input.Color);
	return diffSample;
}