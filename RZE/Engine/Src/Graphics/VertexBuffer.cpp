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
	mGPUBufferHandle = RZE_Application::RZE().GetLegacyRenderer().QueueCreateVertexBufferCommand(static_cast<void*>(mLocalBuffer.data()), sizeof(float), static_cast<U32>(mLocalBuffer.size()));
}
