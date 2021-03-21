#pragma once

#include <vector>

#include <RZE_Config.h>

#include <Utils/PrimitiveDefs.h>

class IndexBuffer
{
public:
	IndexBuffer(const std::vector<U32>& indexData);
	~IndexBuffer() = default;

public:
	void Initialize();

#if WITH_NEW_RENDERER
	const std::vector<U32>& GetData();
#else
	U32 GetGPUBufferIndex();
#endif

private:
	std::vector<U32> mLocalBuffer;
#if !WITH_NEW_RENDERER
	U32 mGPUBufferHandle;
#endif
};

#if WITH_NEW_RENDERER
inline const std::vector<U32>& IndexBuffer::GetData()
{
	return mLocalBuffer;
}
#else
inline U32 IndexBuffer::GetGPUBufferIndex()
{
	return mGPUBufferHandle;
}
#endif
