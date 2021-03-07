#pragma once

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer(const std::vector<float>& localBuffer);
	~VertexBuffer();

	void Initialize();

	U32 GetGPUBufferIndex();

private:
	// The CPU buffer for the mesh data
	std::vector<float> mLocalBuffer;

	// [newrenderer]
	// Don't store this here. VertexBuffer should only be CPU data storage
	// the GPU buffer is tracked renderer side. it will be created when a GPU resource
	// is requested.
	U32 mGPUBufferHandle;
};

inline U32 VertexBuffer::GetGPUBufferIndex()
{
	return mGPUBufferHandle;
}
