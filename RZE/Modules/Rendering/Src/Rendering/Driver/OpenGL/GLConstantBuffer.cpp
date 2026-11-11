#include <Rendering/Driver/OpenGL/GLConstantBuffer.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>
#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <Utils/Memory/MemoryUtils.h>

namespace Rendering
{
	namespace
	{
		void BindUniformBlock(GLShaderTranslator::EStage stage, U32 bufferSlot, U32 buffer)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, GLShaderTranslator::GetUniformBlockBase(stage) + bufferSlot, buffer);
		}
	}

	GLConstantBuffer::GLConstantBuffer(GLDevice& device, U32 alignment)
		: m_device(device)
		, m_alignment(alignment)
	{
	}

	GLConstantBuffer::~GLConstantBuffer()
	{
		Release();
	}

	void GLConstantBuffer::Allocate(const void* /*data*/, size_t size, U32 count)
	{
		// Empty, as the D3D one is made; UpdateData fills it
		m_size = MemoryUtils::AlignSize(size, GetAlignment()) * count;
		glCreateBuffers(1, &m_buffer);
		glNamedBufferStorage(m_buffer, static_cast<GLsizeiptr>(m_size), nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void GLConstantBuffer::Release()
	{
		if (m_buffer != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Buffer, m_buffer);
			m_buffer = 0;
		}
	}

	void GLConstantBuffer::UpdateData(const void* data)
	{
		glNamedBufferSubData(m_buffer, 0, static_cast<GLsizeiptr>(m_size), data);
	}

	void GLConstantBuffer::SetActiveVS(U32 bufferSlot)
	{
		BindUniformBlock(GLShaderTranslator::EStage::Vertex, bufferSlot, m_buffer);
	}

	void GLConstantBuffer::SetActivePS(U32 bufferSlot)
	{
		BindUniformBlock(GLShaderTranslator::EStage::Pixel, bufferSlot, m_buffer);
	}

	void GLConstantBuffer::SetActiveCS(U32 bufferSlot)
	{
		BindUniformBlock(GLShaderTranslator::EStage::Compute, bufferSlot, m_buffer);
	}

	U32 GLConstantBuffer::GetAlignment()
	{
		return m_alignment;
	}
}
