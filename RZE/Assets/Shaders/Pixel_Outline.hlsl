// Outline lines (OutlineRenderStage), from the mask Pixel_OutlineMask drew, blended over the finished image. A pixel is
// on a shape's line when a pixel within its outline's width is in that shape and this pixel isn't, so only the outer
// edge of the shape its objects make together is drawn. Where this pixel is in another shape, only a nearer shape's
// line is drawn on it, so where two shapes touch the line goes on the farther one's side, as against the background.
// Where several lines reach a pixel, the most opaque is drawn.
#include "Common/FullScreenQuad.hlsli"
#include "Common/Outline.hlsli"

// OutlineRenderStage::k_maxOutlines
#define MAX_OUTLINES 8

// Mirrors OutlineRenderStage::OutlineStyleLayout. Padded with scalars: a float3 there would sit off a 16-byte boundary,
// which GLSL's std140 can't express.
struct OutlineStyle
{
	float3 Colour;
	float  Width;
	float  OccludedOpacity;
	float  _pad0;
	float  _pad1;
	float  _pad2;
};

// Mirrors OutlineRenderStage::OutlineBufferLayout
cbuffer OutlineBuffer : register(b0)
{
	OutlineStyle Outlines[MAX_OUTLINES];
	// The mask is the target's size, and the view its top-left part: reads are clamped to the view
	uint2 ViewportSize;
	// How far around each pixel to look, in pixels
	int   SearchRadius;
	uint  _pad0;
};

Texture2D<uint> Mask : register(t0);
// The mask's own depth: which of two touching shapes is nearer
Texture2D<float> MaskDepth : register(t1);

float4 PSMain(PS_IN input) : SV_TARGET
{
	const int2 pixel = int2(input.Position.xy);
	const int2 lastPixel = int2(ViewportSize) - 1;
	const uint ownShape = Mask.Load(int3(pixel, 0)) >> OutlineShapeShift;
	// Only read where this pixel is in a shape, and then only to compare with a touching one's
	const float ownDepth = ownShape != 0u ? MaskDepth.Load(int3(pixel, 0)) : 1.0f;

	float alpha = 0.0f;
	float3 colour = float3(0.0f, 0.0f, 0.0f);
	for (int y = -SearchRadius; y <= SearchRadius; ++y)
	{
		for (int x = -SearchRadius; x <= SearchRadius; ++x)
		{
			const int3 neighbour = int3(clamp(pixel + int2(x, y), int2(0, 0), lastPixel), 0);
			const uint texel = Mask.Load(neighbour);
			const uint shape = texel >> OutlineShapeShift;
			if (shape == 0u || shape == ownShape)
			{
				continue;
			}
			// Next to a farther shape, the line between them is on that one's side: this pixel's shape's, drawn there
			if (ownShape != 0u && MaskDepth.Load(neighbour) > ownDepth)
			{
				continue;
			}

			const uint id = texel & OutlineIDBits;
			const OutlineStyle style = Outlines[id - 1u];
			// Solid within the width, fading out over the half pixel past it, so the line's edge is smooth
			const float coverage = saturate(style.Width + 0.5f - length(float2(x, y)));
			const float opacity = (texel & OutlineInFrontBit) != 0u ? 1.0f : style.OccludedOpacity;
			if (coverage * opacity > alpha)
			{
				alpha = coverage * opacity;
				colour = style.Colour;
			}
		}
	}

	if (alpha <= 0.0f)
	{
		discard;
	}
	return float4(colour, alpha);
}
