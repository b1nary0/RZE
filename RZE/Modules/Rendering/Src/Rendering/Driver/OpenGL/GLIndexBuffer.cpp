#include <Rendering/Driver/OpenGL/GLIndexBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLIndexBuffer::GLIndexBuffer(GLDevice& device)
		: m_device(device)
	{
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		Release();
	}

	void GLIndexBuffer::Allocate(const void* data, size_t size, U32 count)
	{
		m_indexCount = count;
		glCreateBuffers(1, &m_buffer);
		// 32-bit indices, as the D3D one's R32_UINT
		glNamedBufferStorage(m_buffer, static_cast<GLsizeiptr>(size * count), data, 0);
	}

	void GLIndexBuffer::Release()
	{
		if (m_buffer != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Buffer, m_buffer);
			m_buffer = 0;
		}
	}

	void GLIndexBuffer::SetActive()
	{
		m_device.SetIndexBuffer(m_buffer);
	}

	U32 GLIndexBuffer::GetIndexCount()
	{
		return m_indexCount;
	}
}
