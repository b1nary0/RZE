#include <Rendering/Driver/OpenGL/GLTextureBuffer2D.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLTextureBuffer2D::GLTextureBuffer2D(GLDevice& device)
		: m_device(device)
	{
	}

	void GLTextureBuffer2D::Allocate(const void* /*data*/, const GFXTextureBufferParams& params)
	{
		m_params = params;
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLTextureBuffer2D::Release()
	{
	}

	void GLTextureBuffer2D::SetActive(U32 /*textureSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLTextureBuffer2D::SetActiveCS(U32 /*textureSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void* GLTextureBuffer2D::GetTextureData()
	{
		// No GL texture yet: ImGui draws it with none bound
		return nullptr;
	}

	bool GLTextureBuffer2D::IsValid()
	{
		return false;
	}
}
