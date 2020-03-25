struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

cbuffer ViewProjectionBuf : register(b0)
{
	matrix ViewProj;
};

cbuffer ModelMatBuf : register(b1)
{
	matrix ModelMat;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR)
{
    VS_OUTPUT output;

	float4 transformedPos = inPos;

    transformedPos = mul(ModelMat, transformedPos);
    transformedPos = mul(ViewProj, transformedPos);

    output.Pos = transformedPos;
    output.Color = inColor;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}