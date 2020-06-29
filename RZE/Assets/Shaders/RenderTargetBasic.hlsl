struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoords : UV;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float2 TexCoords : UV;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    output.Pos = float4(input.Pos, 1.0f);
    output.TexCoords = input.TexCoords;

    return output;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 TexCoords : UV;
};

Texture2D tex : register(t0);
SamplerState s1 : register(s0);

float4 PS(PS_INPUT input) : SV_TARGET
{
	float4 sample = tex.Sample(s1, input.TexCoords);
    return sample;
}