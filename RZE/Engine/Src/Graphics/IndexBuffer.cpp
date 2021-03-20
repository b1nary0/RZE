#include <StdAfx.h>
#include <Graphics/IndexBuffer.h>

IndexBuffer::IndexBuffer(const std::vector<U32>& indexData)
	: mLocalBuffer(indexData)
{
}

void IndexBuffer::Initialize()
{
#if WITH_NEW_RENDERER
#else
	mGPUBufferHandle = RZE_Application::RZE().GetLegacyRenderer().QueueCreateIndexBufferCommand(static_cast<void*>(mLocalBuffer.data()), sizeof(U32), static_cast<U32>(mLocalBuffer.size()));
#endif
}

