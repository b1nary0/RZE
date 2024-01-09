#pragma once

namespace Rendering
{
	enum class EPrimitiveTopology
	{
		TriangleList,
		TriangleStrip
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