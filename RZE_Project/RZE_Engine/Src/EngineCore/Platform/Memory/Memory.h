#pragma once

#define ALLOCATOR_DEFAULT_FIXED_SIZE_KB 64 * 1024 // 64KB
#define ALLOCATOR_BLOCK_DEFAULT_MAX_SIZE 200 * 1024 * 1024 // 64MB
#define ALLOCATOR_BLOCK_DEFAULT_ALIGNMENT 32

class IAllocator
{
public:
    virtual void Init() = 0;

    virtual void* Allocate(size_t size) = 0;
    virtual void Free(void* ptr) = 0;

private:
};
