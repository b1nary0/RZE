#include <Rendering/Driver/OpenGL/GLVertexBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLVertexBuffer::GLVertexBuffer(GLDevice& device)
		: m_device(device)
	{
	}

	void GLVertexBuffer::SetDynamic(bool isDynamic)
	{
		m_isDynamic = isDynamic;
	}

	void GLVertexBuffer::Allocate(const void* /*data*/, size_t /*size*/, U32 /*count*/, U32 /*stride*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLVertexBuffer::Release()
	{
	}

	void GLVertexBuffer::UpdateData(const void* /*data*/, size_t /*bytes*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLVertexBuffer::SetActive(U32 /*bufferSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}
}
