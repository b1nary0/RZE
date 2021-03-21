struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 FragPos : POSITION;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
	float yVal = 0.15f + (input.FragPos.y * 0.15f);
    return float4(yVal, yVal, yVal, 1.0f);
}