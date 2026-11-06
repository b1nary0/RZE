#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>

namespace MemoryUtils
{
	inline size_t Kilobytes(size_t val)
	{
		return val * 1024;
	}

	inline size_t Megabytes(size_t val)
	{
		return Kilobytes(val) * 1024;
	}

	inline size_t Gigabytes(size_t val)
	{
		return Megabytes(val) * 1024;
	}

	// Rounds size up to a multiple of boundary, which must be a power of 2
	size_t AlignSize(size_t size, size_t boundary);
}