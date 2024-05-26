struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 Colour : COLOUR;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
    return float4(input.Colour, 1.0f);
}