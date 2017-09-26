#pragma once

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
	virtual void Init() = 0;

	virtual void* Allocate(size_t size) = 0;
	virtual void Free(void* ptr) = 0;

private:
};
