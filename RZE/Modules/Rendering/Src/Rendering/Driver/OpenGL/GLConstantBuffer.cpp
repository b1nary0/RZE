#include <Rendering/Driver/OpenGL/GLConstantBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLConstantBuffer::GLConstantBuffer(GLDevice& device, U32 alignment)
		: m_device(device)
		, m_alignment(alignment)
	{
	}

	void GLConstantBuffer::Allocate(const void* /*data*/, size_t /*size*/, U32 /*count*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLConstantBuffer::Release()
	{
	}

	void GLConstantBuffer::UpdateData(const void* /*data*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLConstantBuffer::SetActiveVS(U32 /*bufferSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLConstantBuffer::SetActivePS(U32 /*bufferSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLConstantBuffer::SetActiveCS(U32 /*bufferSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	U32 GLConstantBuffer::GetAlignment()
	{
		return m_alignment;
	}
}
