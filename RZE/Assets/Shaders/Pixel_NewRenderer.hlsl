struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 FragPos : POSITION;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
    return float4(0.25f, 0.25f, 1.0f, 1.0f);
}