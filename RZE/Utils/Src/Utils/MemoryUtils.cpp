#include <Utils/MemoryUtils.h>

U32 MemoryUtils::AlignSize(U32 size, U32 boundary)
{
	return (size + boundary) & ~boundary;
}
