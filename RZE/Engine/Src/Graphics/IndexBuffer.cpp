#include <StdAfx.h>
#include <Graphics/IndexBuffer.h>

#include <Rendering/Renderer.h>

IndexBuffer::IndexBuffer()
{
}

void IndexBuffer::Initialize(const std::vector<U32>& indexData)
{
	m_gpuBuffer = Rendering::Renderer::CreateIndexBuffer(indexData.data(), sizeof(U32), indexData.size());
}

