#pragma once

namespace MemoryUtils
{
	U32 AlignSize(U32 size, U32 boundary)
	{
		return (size + boundary) & ~boundary;
	}
}