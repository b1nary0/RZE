#pragma once

#include <Utils/PrimitiveDefs.h>

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

	U32 AlignSize(U32 size, U32 boundary);
}