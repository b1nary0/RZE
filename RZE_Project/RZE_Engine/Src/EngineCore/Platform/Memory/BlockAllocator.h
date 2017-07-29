#pragma once

#include <EngineCore/Platform/Memory/Memory.h>

/*
 * Using the Buddy Block allocation algorithm.
 */

class BlockAllocator : public IAllocator
{
public:
    BlockAllocator(size_t blockSizeKB = ALLOCATOR_DEFAULT_FIXED_SIZE_KB, size_t alignment = ALLOCATOR_BLOCK_DEFAULT_ALIGNMENT);

    virtual void Init() override;

    virtual void* Allocate(size_t size) override;
    virtual void  Free(void* ptr) override;

private:
    struct BlockHeader
    {

    };

    struct MemoryBlock
    {
        U16 mSize;
        char* mMemory;
    };

    char* mPool;
    size_t mBlockSizeKB;
    size_t mAlignment;

    char* mStackPtr; // tmp for testing
};
