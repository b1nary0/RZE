#pragma once

#include <Rendering/BufferHandle.h>

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	// Uploads vertexData to a new GPU buffer. The upload takes its own copy, so vertexData can go once this returns.
	void Initialize(const std::vector<float>& vertexData, U32 stride);

	const Rendering::VertexBufferHandle& GetPlatformObject() const { return m_gpuBuffer; }

private:
	// Should we also store the layout or leave that to Renderer?
	// We still need to know what that layout is when we form this buffer
	Rendering::VertexBufferHandle m_gpuBuffer;
};
