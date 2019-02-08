float4 VSMain(float3 pos : POSITION) : SV_POSITION // main is the default function name
{
    return float4(pos, 1.0f);
}