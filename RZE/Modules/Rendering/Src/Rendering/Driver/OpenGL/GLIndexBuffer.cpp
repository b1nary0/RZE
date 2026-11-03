#include <Rendering/Driver/OpenGL/GLIndexBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLIndexBuffer::GLIndexBuffer(GLDevice& device)
		: m_device(device)
	{
	}

	void GLIndexBuffer::Allocate(const void* /*data*/, size_t /*size*/, U32 count)
	{
		m_indexCount = count;
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLIndexBuffer::Release()
	{
	}

	void GLIndexBuffer::SetActive()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	U32 GLIndexBuffer::GetIndexCount()
	{
		return m_indexCount;
	}
}
