struct PS_IN
{
	float4 Position   : SV_POSITION;
	float4 Colour     : COLOUR;
	float3 WorldPos   : WORLDPOS;
	float3 FadeCentre : FADECENTRE;
	// x: fully visible within this distance of FadeCentre; y: gone beyond it. y == 0 means no fade.
	float2 FadeRange  : FADERANGE;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
	float alpha = input.Colour.a;
	if (input.FadeRange.y > 0.0f)
	{
		alpha *= 1.0f - smoothstep(input.FadeRange.x, input.FadeRange.y, distance(input.WorldPos, input.FadeCentre));
	}

	// Fully faded pixels would still write depth and could hide lines drawn after them
	clip(alpha - 0.001f);

	return float4(input.Colour.rgb, alpha);
}
