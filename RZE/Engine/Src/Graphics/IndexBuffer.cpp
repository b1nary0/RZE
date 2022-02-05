#include <StdAfx.h>
#include <Graphics/IndexBuffer.h>

#include <Rendering/Renderer.h>

IndexBuffer::IndexBuffer()
{
}

void IndexBuffer::Initialize(const std::vector<U32>& indexData)
{
	m_localBuffer = std::move(indexData);
	m_gpuBuffer = Rendering::Renderer::CreateIndexBuffer(m_localBuffer.data(), sizeof(U32), m_localBuffer.size());
}

