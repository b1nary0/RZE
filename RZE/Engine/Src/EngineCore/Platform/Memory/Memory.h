#pragma once

// NOTE(Josh):
// When an allocator solution finally arrives, these will help locate where we are responsible for allocations.
// Will need to retroactively attack this, and use this going forward.
#define RZE_NEW new;
#define RZE_DEL delete;

inline size_t Kilobyes(size_t val)
{
	return val * 1024;
}

inline size_t Megabytes(size_t val)
{
	return Kilobyes(val) * 1024;
}

inline size_t Gigabytes(size_t val)
{
	return Megabytes(val) * 1024;
}

#define ALLOCATOR_DEFAULT_FIXED_SIZE_KB Kilobyes(64)
#define ALLOCATOR_BLOCK_DEFAULT_MAX_SIZE Megabytes(200)
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
