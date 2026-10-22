// FXAA: smooths jagged edges in the tonemapped, sRGB-encoded image (Pixel_Tonemap's output). A port of Timothy Lottes'
// FXAA 3.11 Quality: find an edge by luma contrast, search along it for both ends of the stair-step, and move the
// sample across the edge by where the pixel sits on that step, so long shallow edges blend smoothly. Detail thinner
// than a pixel is softened as well. Pixels with little contrast around them pass through, so texture detail stays
// sharp. How far it searches, how faint an edge counts and how much it softens come from the view's level
// (PostProcessParams, AntiAliasingSettings).
#include "Common/PostProcess.hlsli"

Texture2D DisplayTexture : register(t0);
// Linear filtering: bound with the texture (DX11TextureBuffer2D::SetActive)
SamplerState LinearSampler : register(s0);

float Luma(float3 colour)
{
	return dot(colour, float3(0.299f, 0.587f, 0.114f));
}

struct Frame
{
	float2 Texel;
	// The viewport is the texture's top-left corner; the sampler wraps, so samples are clamped inside it
	float2 UVMin;
	float2 UVMax;
};

float3 SampleColour(Frame frame, float2 uv)
{
	return DisplayTexture.SampleLevel(LinearSampler, clamp(uv, frame.UVMin, frame.UVMax), 0.0f).rgb;
}

float SampleLuma(Frame frame, float2 uv)
{
	return Luma(SampleColour(frame, uv));
}

float SampleLumaOffset(Frame frame, float2 uv, float2 offset)
{
	return SampleLuma(frame, uv + offset * frame.Texel);
}

float SearchStep(uint step)
{
	const float4 steps = SearchSteps[step >> 2];
	const uint component = step & 3;
	return component == 0 ? steps.x : component == 1 ? steps.y : component == 2 ? steps.z : steps.w;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
	Frame frame;
	float2 textureSize;
	DisplayTexture.GetDimensions(textureSize.x, textureSize.y);
	frame.Texel = 1.0f / textureSize;
	frame.UVMin = 0.5f * frame.Texel;
	frame.UVMax = ViewportScale - 0.5f * frame.Texel;

	// Texel centres
	float2 posM = input.Position.xy * frame.Texel;

	const float3 colourM = SampleColour(frame, posM);
	const float lumaM = Luma(colourM);
	float lumaS = SampleLumaOffset(frame, posM, float2(0.0f, 1.0f));
	const float lumaE = SampleLumaOffset(frame, posM, float2(1.0f, 0.0f));
	float lumaN = SampleLumaOffset(frame, posM, float2(0.0f, -1.0f));
	const float lumaW = SampleLumaOffset(frame, posM, float2(-1.0f, 0.0f));

	// Not enough contrast around it to be an edge
	const float rangeMax = max(max(lumaN, lumaW), max(lumaE, max(lumaS, lumaM)));
	const float rangeMin = min(min(lumaN, lumaW), min(lumaE, min(lumaS, lumaM)));
	const float range = rangeMax - rangeMin;
	if (range < max(EdgeThresholdMin, rangeMax * EdgeThreshold))
	{
		return float4(colourM, 1.0f);
	}

	const float lumaNW = SampleLumaOffset(frame, posM, float2(-1.0f, -1.0f));
	const float lumaSE = SampleLumaOffset(frame, posM, float2(1.0f, 1.0f));
	const float lumaNE = SampleLumaOffset(frame, posM, float2(1.0f, -1.0f));
	const float lumaSW = SampleLumaOffset(frame, posM, float2(-1.0f, 1.0f));

	// Is the edge horizontal or vertical? Compare the second differences across each way.
	const float lumaNS = lumaN + lumaS;
	const float lumaWE = lumaW + lumaE;
	const float lumaNESE = lumaNE + lumaSE;
	const float lumaNWNE = lumaNW + lumaNE;
	const float lumaNWSW = lumaNW + lumaSW;
	const float lumaSWSE = lumaSW + lumaSE;
	const float edgeHorizontal = abs(-2.0f * lumaW + lumaNWSW) + abs(-2.0f * lumaM + lumaNS) * 2.0f + abs(-2.0f * lumaE + lumaNESE);
	const float edgeVertical = abs(-2.0f * lumaS + lumaSWSE) + abs(-2.0f * lumaM + lumaWE) * 2.0f + abs(-2.0f * lumaN + lumaNWNE);
	const bool isHorizontal = edgeHorizontal >= edgeVertical;

	// Sub-pixel softening: how much the pixel stands out from the average of its 3x3 neighbourhood
	const float subpixelAverage = ((lumaNS + lumaWE) * 2.0f + lumaNWSW + lumaNESE) * (1.0f / 12.0f);
	const float subpixelContrast = saturate(abs(subpixelAverage - lumaM) / range);
	const float subpixelSmooth = (-2.0f * subpixelContrast + 3.0f) * subpixelContrast * subpixelContrast;
	const float subpixelOffset = subpixelSmooth * subpixelSmooth * Subpixel;

	// Which side of the pixel the edge is on: the neighbour across the edge with the bigger luma step
	if (!isHorizontal)
	{
		lumaN = lumaW;
		lumaS = lumaE;
	}
	float stepLength = isHorizontal ? frame.Texel.y : frame.Texel.x;
	const float gradientN = lumaN - lumaM;
	const float gradientS = lumaS - lumaM;
	const bool isPairN = abs(gradientN) >= abs(gradientS);
	const float gradient = max(abs(gradientN), abs(gradientS));
	if (isPairN)
	{
		stepLength = -stepLength;
	}
	// The average luma of the pixel and its neighbour across the edge
	const float lumaEdge = (isPairN ? lumaN + lumaM : lumaS + lumaM) * 0.5f;

	// Start half a pixel across, on the edge itself, and search along it both ways
	float2 posB = posM;
	if (isHorizontal)
	{
		posB.y += stepLength * 0.5f;
	}
	else
	{
		posB.x += stepLength * 0.5f;
	}
	const float2 alongEdge = isHorizontal ? float2(frame.Texel.x, 0.0f) : float2(0.0f, frame.Texel.y);

	float2 posN = posB - alongEdge * SearchStep(0);
	float2 posP = posB + alongEdge * SearchStep(0);
	float lumaEndN = SampleLuma(frame, posN) - lumaEdge;
	float lumaEndP = SampleLuma(frame, posP) - lumaEdge;
	// An end is where the luma along the edge has moved far enough from the edge's own
	const float gradientScaled = gradient * 0.25f;
	bool isDoneN = abs(lumaEndN) >= gradientScaled;
	bool isDoneP = abs(lumaEndP) >= gradientScaled;

	const uint stepCount = (uint)SearchStepCount;
	[loop]
	for (uint step = 1; step < stepCount; ++step)
	{
		if (!isDoneN)
		{
			posN -= alongEdge * SearchStep(step);
		}
		if (!isDoneP)
		{
			posP += alongEdge * SearchStep(step);
		}
		// FXAA takes the last step without looking: the end is at least that far
		if ((isDoneN && isDoneP) || step == stepCount - 1)
		{
			break;
		}

		if (!isDoneN)
		{
			lumaEndN = SampleLuma(frame, posN) - lumaEdge;
			isDoneN = abs(lumaEndN) >= gradientScaled;
		}
		if (!isDoneP)
		{
			lumaEndP = SampleLuma(frame, posP) - lumaEdge;
			isDoneP = abs(lumaEndP) >= gradientScaled;
		}
	}

	// Where the pixel sits along the stair-step: near an end it moves furthest across the edge
	const float distanceN = isHorizontal ? posM.x - posN.x : posM.y - posN.y;
	const float distanceP = isHorizontal ? posP.x - posM.x : posP.y - posM.y;
	const bool isNearerN = distanceN < distanceP;
	const float distance = min(distanceN, distanceP);
	const float spanLength = distanceN + distanceP;

	// Only blend toward an end whose luma moves the same way as the pixel's from the edge; otherwise the pixel isn't
	// on that step's slope
	const bool isLumaMBelowEdge = lumaM - lumaEdge < 0.0f;
	const bool isGoodSpan = isNearerN ? (lumaEndN < 0.0f) != isLumaMBelowEdge : (lumaEndP < 0.0f) != isLumaMBelowEdge;
	const float edgeOffset = isGoodSpan ? 0.5f - distance / spanLength : 0.0f;

	const float offset = max(edgeOffset, subpixelOffset);
	if (isHorizontal)
	{
		posM.y += offset * stepLength;
	}
	else
	{
		posM.x += offset * stepLength;
	}
	return float4(SampleColour(frame, posM), 1.0f);
}
