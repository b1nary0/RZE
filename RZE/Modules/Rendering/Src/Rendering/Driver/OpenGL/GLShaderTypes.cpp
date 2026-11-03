#include <Rendering/Driver/OpenGL/GLShaderTypes.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

namespace Rendering
{
	GLVertexShader::GLVertexShader(GLDevice& device)
		: m_device(device)
	{
	}

	void GLVertexShader::Create(const Filepath& /*filePath*/, const ShaderInputLayout& /*inputLayout*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLVertexShader::Release()
	{
	}

	void GLVertexShader::SetActive()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLVertexShader::SetInputLayout()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	GLPixelShader::GLPixelShader(GLDevice& device)
		: m_device(device)
	{
	}

	void GLPixelShader::Create(const Filepath& /*filePath*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLPixelShader::Release()
	{
	}

	void GLPixelShader::SetActive()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	GLComputeShader::GLComputeShader(GLDevice& device)
		: m_device(device)
	{
	}

	void GLComputeShader::Create(const Filepath& /*filePath*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLComputeShader::Release()
	{
	}

	void GLComputeShader::SetActive()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}
}
