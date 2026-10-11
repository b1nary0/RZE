struct PS_IN
{
	float4 Position : SV_POSITION;
	float4 Colour   : COLOUR;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
	return input.Colour;
}
