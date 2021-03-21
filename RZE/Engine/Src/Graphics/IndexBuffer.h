#pragma once

#include <RZE_Config.h>

#include <vector>

#include <Utils/PrimitiveDefs.h>

class IndexBuffer
{
public:
	IndexBuffer(const std::vector<U32>& indexData);
	~IndexBuffer() = default;

public:
	void Initialize();

	inline const std::vector<U32>& GetData() const { return mLocalBuffer; }

#if !WITH_NEW_RENDERER
	U32 GetGPUBufferIndex();
#endif

private:
	std::vector<U32> mLocalBuffer;
#if !WITH_NEW_RENDERER
	U32 mGPUBufferHandle;
#endif
};

#if !WITH_NEW_RENDERER
inline U32 IndexBuffer::GetGPUBufferIndex()
{
	return mGPUBufferHandle;
}
#endif
