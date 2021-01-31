#include <Utils/MemoryUtils.h>

#include <Utils/DebugUtils/Debug.h>

U32 MemoryUtils::AlignSize(U32 size, U32 boundary)
{
	AssertMsg(boundary > 0, "Boundary must be greater than 0");
	AssertMsg((boundary & (boundary - 1)) == 0, "Boundary must be power of 2");

	U32 bnd = boundary - 1;
	U32 alignedSize = (size + bnd) & ~bnd;
	return alignedSize;
}
