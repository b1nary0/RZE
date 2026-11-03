#include <Rendering/Driver/OpenGL/GLVertexBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLVertexBuffer::GLVertexBuffer(GLDevice& device)
		: m_device(device)
	{
	}

	GLVertexBuffer::~GLVertexBuffer()
	{
		Release();
	}

	void GLVertexBuffer::SetDynamic(bool isDynamic)
	{
		m_isDynamic = isDynamic;
	}

	void GLVertexBuffer::Allocate(const void* data, size_t size, U32 count, U32 stride)
	{
		m_stride = stride;
		m_capacityBytes = size * count;

		glCreateBuffers(1, &m_buffer);
		// A dynamic buffer starts empty and is written with UpdateData, as the D3D one is
		glNamedBufferStorage(m_buffer, static_cast<GLsizeiptr>(m_capacityBytes), data, m_isDynamic ? GL_DYNAMIC_STORAGE_BIT : 0);
	}

	void GLVertexBuffer::Release()
	{
		if (m_buffer != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Buffer, m_buffer);
			m_buffer = 0;
		}
	}

	void GLVertexBuffer::UpdateData(const void* data, size_t bytes)
	{
		AssertExpr(m_isDynamic);
		AssertExpr(bytes <= m_capacityBytes);
		glNamedBufferSubData(m_buffer, 0, static_cast<GLsizeiptr>(bytes), data);
	}

	void GLVertexBuffer::SetActive(U32 bufferSlot)
	{
		m_device.SetVertexBuffer(*this, bufferSlot);
	}
}
