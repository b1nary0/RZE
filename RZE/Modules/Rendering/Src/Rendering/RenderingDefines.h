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
		AlphaBlend  // Straight alpha: src * a + dst * (1 - a)
	};

	enum class EDepthState
	{
		ReadWrite, // Tests and writes depth; what every pass expects unless it asks otherwise
		ReadOnly,  // Tests but doesn't write, so see-through surfaces don't hide each other (Docs/Plans/Transparency.md)
		Off        // Neither tests nor writes
	};

	struct ViewportParams
	{
		float Width;
		float Height;
		float MinDepth;
		float MaxDepth;
		float TopLeftX;
		float TopLeftY;
	};
}