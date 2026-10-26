#pragma once

#include <Utils/PrimitiveDefs.h>

#include <atomic>

namespace Rendering
{
	// One texel copied back from the GPU by Renderer::ReadTexel. It's filled in on the render thread;
	// poll IsReady from any thread, and read Value once it's true.
	struct TexelReadback
	{
		std::atomic<bool> IsReady = false;
		// The texel's bytes, zero-extended: e.g. a 4-byte texel fills the low 32 bits
		U64 Value = 0;
	};
}
