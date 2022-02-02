#include <StdAfx.h>
#include <Graphics/VertexBuffer.h>

VertexBuffer::VertexBuffer()
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Initialize(const std::vector<float>& localBuffer)
{
	m_localBuffer = std::move(localBuffer);
}
