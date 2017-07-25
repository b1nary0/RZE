#pragma once

#include <EngineCore/Platform/Memory/Memory.h>

class BlockAllocator : public IAllocator
{
public:
    BlockAllocator(size_t blockSizeKB = ALLOCATOR_DEFAULT_FIXED_SIZE_KB);

    virtual void Init() override;

    virtual void* Allocate(size_t size) override;
    virtual void  Free(void* ptr) override;

private:
    char* mPool;
    size_t mBlockSizeKB;

    char* mStackPtr; // tmp for testing
};
