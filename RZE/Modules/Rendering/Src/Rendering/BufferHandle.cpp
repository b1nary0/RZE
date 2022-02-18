#include <Rendering/BufferHandle.h>

#include <Rendering/Driver/GFXBuffer.h>

void* Rendering::TextureBuffer2DHandle::GetTextureData() const
{
	return m_buffer->GetTextureData();
}
