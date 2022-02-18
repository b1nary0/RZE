#include <StdAfx.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>

VertexBuffer::VertexBuffer()
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Initialize(std::vector<float>&& localBuffer, U32 stride)
{
	m_localBuffer = std::move(localBuffer);
	m_gpuBuffer = Rendering::Renderer::CreateVertexBuffer(m_localBuffer.data(), sizeof(float), m_localBuffer.size(), stride);
}
