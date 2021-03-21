#pragma once
#include <RZE_Config.h>

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer(const std::vector<float>& localBuffer);
	~VertexBuffer();

	void Initialize();

	inline const std::vector<float>& GetData() const { return mLocalBuffer; }

#if !WITH_NEW_RENDERER
	U32 GetGPUBufferIndex();
#endif

private:
	// The CPU buffer for the mesh data
	std::vector<float> mLocalBuffer;

	// [newrenderer]
	// Don't store this here. VertexBuffer should only be CPU data storage
	// the GPU buffer is tracked renderer side. it will be created when a GPU resource
	// is requested.
#if !WITH_NEW_RENDERER
	U32 mGPUBufferHandle;
#endif
};

#if !WITH_NEW_RENDERER
inline U32 VertexBuffer::GetGPUBufferIndex()
{
	return mGPUBufferHandle;
}
#endif
