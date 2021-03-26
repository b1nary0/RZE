#include <StdAfx.h>
#include <Graphics/VertexBuffer.h>

VertexBuffer::VertexBuffer(const std::vector<float>& localBuffer)
	: mLocalBuffer(std::move(localBuffer))
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Initialize()
{
}
