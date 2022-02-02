#pragma once
#include <RZE_Config.h>

// @TODO remove when VertexBufferHandle gets moved to its own file
#include <Rendering/Renderer.h>

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();

	void Initialize(std::vector<float>&& localBuffer);

	inline const std::vector<float>& GetData() const { return m_localBuffer; }

private:
	// Should we also store the layout or leave that to Renderer?
	// We still need to know what that layout is when we form this buffer
	Rendering::VertexBufferHandle m_gpuBuffer;
	// The CPU buffer for the mesh data
	std::vector<float> m_localBuffer;
};
