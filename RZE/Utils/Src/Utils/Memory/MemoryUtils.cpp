#include <Utils/Memory/MemoryUtils.h>

#include <Utils/DebugUtils/Debug.h>

size_t MemoryUtils::AlignSize(size_t size, size_t boundary)
{
	AssertMsg(boundary > 0, "Boundary must be greater than 0");
	AssertMsg((boundary & (boundary - 1)) == 0, "Boundary must be power of 2");

	const size_t bnd = boundary - 1;
	const size_t alignedSize = (size + bnd) & ~bnd;
	return alignedSize;
}
