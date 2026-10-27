#pragma once

namespace Rendering
{
	enum class EPrimitiveTopology
	{
		TriangleList,
		TriangleStrip,
		LineList
	};

	enum class ERasterizerState
	{
		Default,      // Back-face culling
		ShadowCaster  // No culling (thin geometry still casts) + slope-scaled depth bias
	};

	enum class EBlendState
	{
		Opaque,     // Blending off; what every pass expects unless it asks otherwise
		AlphaBlend,    // Straight alpha: src * a + dst * (1 - a)
		Premultiplied, // Colour already multiplied by its alpha: src + dst * (1 - a)
		Additive,      // src * a + dst
		Multiply       // src * dst
		// Each leaves the target's alpha as it was (see ForwardRenderStage's note on empty background)
	};

	enum class EDepthState
	{
		ReadWrite, // Tests and writes depth; what every pass expects unless it asks otherwise
		ReadOnly,  // Tests but doesn't write, so see-through surfaces don't hide each other (Docs/Plans/Transparency.md)
		// Passes where depth equals what's there too, without writing: for drawing over a depth prepass, so each pixel is
		// shaded only by the surface the prepass left nearest
		ReadOnlyLessEqual,
		Off        // Neither tests nor writes
	};

	// By default the full depth range from the target's top-left corner, so most passes only give the size:
	// SetViewport({ width, height })
	struct ViewportParams
	{
		float Width = 0.0f;
		float Height = 0.0f;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
	};
}