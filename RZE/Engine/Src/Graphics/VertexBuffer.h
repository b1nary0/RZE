#pragma once

#include <Rendering/BufferHandle.h>

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	void Initialize(std::vector<float>&& localBuffer, U32 stride);

	const std::vector<float>& GetData() const { return m_localBuffer; }

	const Rendering::VertexBufferHandle GetPlatformObject() const { return m_gpuBuffer; }

private:
	// Should we also store the layout or leave that to Renderer?
	// We still need to know what that layout is when we form this buffer
	Rendering::VertexBufferHandle m_gpuBuffer;
	// The CPU buffer for the mesh data
	std::vector<float> m_localBuffer;
};
