#include <StdAfx.h>
#include <Graphics/IndexBuffer.h>

IndexBuffer::IndexBuffer(const std::vector<U32>& indexData)
	: mLocalBuffer(indexData)
{
}

void IndexBuffer::Initialize()
{
	mGPUBufferHandle = RZE_Application::RZE().GetRenderer().QueueCreateIndexBufferCommand(static_cast<void*>(mLocalBuffer.data()), static_cast<U32>(mLocalBuffer.size()));
}

