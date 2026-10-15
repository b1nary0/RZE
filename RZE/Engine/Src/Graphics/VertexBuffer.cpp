#include <StdAfx.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>

VertexBuffer::VertexBuffer()
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Initialize(const std::vector<float>& vertexData, U32 stride)
{
	m_gpuBuffer = Rendering::Renderer::CreateVertexBuffer(vertexData.data(), sizeof(float), vertexData.size(), stride);
}
