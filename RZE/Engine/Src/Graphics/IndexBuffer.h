#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Rendering/BufferHandle.h>

#include <vector>

class IndexBuffer
{
public:
	IndexBuffer();
	~IndexBuffer() = default;

public:
	// Uploads indexData to a new GPU buffer. The upload takes its own copy, so indexData can go once this returns.
	void Initialize(const std::vector<U32>& indexData);

	const Rendering::IndexBufferHandle& GetPlatformObject() const { return m_gpuBuffer; }

private:
	Rendering::IndexBufferHandle m_gpuBuffer;
};
