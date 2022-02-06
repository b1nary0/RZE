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
	void Initialize(const std::vector<U32>& indexData);

	const std::vector<U32>& GetData() const { return m_localBuffer; }

	const Rendering::IndexBufferHandle GetPlatformObject() const { return m_gpuBuffer; }

private:
	Rendering::IndexBufferHandle m_gpuBuffer;
	std::vector<U32> m_localBuffer;
};
