#include <StdAfx.h>

#include <EngineCore/Platform/Memory/BlockAllocator.h>

BlockAllocator::BlockAllocator(size_t blockSizeKB /*= ALLOCATOR_DEFAULT_FIXED_SIZE_KB*/)
{
    mBlockSizeKB = blockSizeKB;
}

void BlockAllocator::Init()
{
    mPool = static_cast<char*>(malloc(ALLOCATOR_BLOCK_DEFAULT_MAX_SIZE));
    memset(mPool, 0, mBlockSizeKB);

    mStackPtr = &mPool[0];
}

void* BlockAllocator::Allocate(size_t size)
{
    void* ptr = mStackPtr;
    mStackPtr = mStackPtr + size;

    return ptr;
}

void BlockAllocator::Free(void* ptr)
{

}
