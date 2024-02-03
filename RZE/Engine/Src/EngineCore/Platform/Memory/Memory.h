#pragma once

#include <Utils/Memory/MemoryUtils.h>

// NOTE(Josh):
// When an allocator solution finally arrives, these will help locate where we are responsible for allocations.
// Will need to retroactively attack this, and use this going forward.
#define RZE_NEW new;
#define RZE_DEL delete;

#define ALLOCATOR_DEFAULT_FIXED_SIZE_KB MemoryUtils::Kilobytes(64)
#define ALLOCATOR_BLOCK_DEFAULT_MAX_SIZE MemoryUtils::Megabytes(200)
#define ALLOCATOR_BLOCK_DEFAULT_ALIGNMENT 32

class IAllocator
{
public:
	IAllocator() = default;
	virtual ~IAllocator() = default;

	virtual void Init() = 0;

	virtual void* Allocate(size_t size) = 0;
	virtual void Free(void* ptr) = 0;

private:
};
