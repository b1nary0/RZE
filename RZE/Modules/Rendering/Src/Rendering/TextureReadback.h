#pragma once

#include <Utils/PrimitiveDefs.h>

#include <atomic>
#include <vector>

namespace Rendering
{
	// A whole texture (its most detailed mip) copied back from the GPU by Renderer::ReadTexture. It's filled in on
	// the render thread; poll IsReady from any thread, and read the rest once it's true.
	struct TextureReadback
	{
		std::atomic<bool> IsReady = false;
		U32 Width = 0;
		U32 Height = 0;
		// In the texture's own format, e.g. 4 for RGBA8
		U32 BytesPerTexel = 0;
		// Row by row from the top, tightly packed: each row is Width * BytesPerTexel bytes
		std::vector<U8> Data;
	};
}
