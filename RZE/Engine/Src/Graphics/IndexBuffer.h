#pragma once

#include <RZE_Config.h>

#include <Utils/PrimitiveDefs.h>

// @TODO remove when IndexBufferHandle gets moved to its own file
#include <Rendering/Renderer.h>

#include <vector>

class IndexBuffer
{
public:
	IndexBuffer();
	~IndexBuffer() = default;

public:
	void Initialize(const std::vector<U32>& indexData);

	inline const std::vector<U32>& GetData() const { return m_localBuffer; }

private:
	Rendering::IndexBufferHandle m_gpuBuffer;
	std::vector<U32> m_localBuffer;
};
