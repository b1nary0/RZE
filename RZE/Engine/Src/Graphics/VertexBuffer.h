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
	U32 mGPUBufferHandle;
};

inline U32 VertexBuffer::GetGPUBufferIndex()
{
	return mGPUBufferHandle;
}
