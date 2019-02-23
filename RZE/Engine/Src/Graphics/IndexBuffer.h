#pragma once

#include <vector>

#include <Utils/PrimitiveDefs.h>

class IndexBuffer
{
public:
	IndexBuffer(const std::vector<U32>& indexData);
	~IndexBuffer() = default;

public:
	void Initialize();

private:
	std::vector<U32> mLocalBuffer;
	U32 mGPUBufferHandle;
};