#ifndef COLOURSPACE_HLSLI
#define COLOURSPACE_HLSLI

// Textures and render targets are UNORM holding sRGB-encoded data, so convert manually.
float3 SRGBToLinear(float3 colour)
{
	return pow(max(colour, 0.0f), 2.2f);
}

float3 LinearToSRGB(float3 colour)
{
	return pow(max(colour, 0.0f), 1.0f / 2.2f);
}

// Rec. 709 relative luminance of a linear colour
float Luminance(float3 linearColour)
{
	return dot(linearColour, float3(0.2126f, 0.7152f, 0.0722f));
}

// ACES filmic curve fit (Krzysztof Narkowicz). Keeps more contrast and saturation than Reinhard.
float3 TonemapACES(float3 colour)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return saturate((colour * (a * colour + b)) / (colour * (c * colour + d) + e));
}

#endif // COLOURSPACE_HLSLI
