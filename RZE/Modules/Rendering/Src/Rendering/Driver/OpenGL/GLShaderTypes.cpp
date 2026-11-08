#include <Rendering/Driver/OpenGL/GLShaderTypes.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLDevice.h>

#include <Utils/Platform/Filepath.h>

#include <algorithm>
#include <string>

namespace Rendering
{
	namespace
	{
		GLenum ToGLShaderType(GLShaderTranslator::EStage stage)
		{
			switch (stage)
			{
			case GLShaderTranslator::EStage::Pixel: return GL_FRAGMENT_SHADER;
			case GLShaderTranslator::EStage::Compute: return GL_COMPUTE_SHADER;
			case GLShaderTranslator::EStage::Vertex:
			default: return GL_VERTEX_SHADER;
			}
		}

		// Without its trailing newlines, since the log adds its own
		// Only the log uses it, which Retail builds leave out
		[[maybe_unused]] std::string TrimEnd(std::string text)
		{
			while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
			{
				text.pop_back();
			}
			return text;
		}
	}

	GLShaderProgram::GLShaderProgram(GLDevice& device, GLShaderTranslator::EStage stage)
		: m_device(device)
		, m_stage(stage)
	{
	}

	GLShaderProgram::~GLShaderProgram()
	{
		Release();
	}

	bool GLShaderProgram::Create(const Filepath& filePath)
	{
		Release();

		std::string errors;
		if (!GLShaderTranslator::Translate(filePath, m_stage, m_translation, errors))
		{
			// As the DX11 backend reports a shader that doesn't compile
			RZE_LOG_ERROR("Couldn't compile shader [" << filePath.GetRelativePath() << "]:\n" << TrimEnd(errors));
			return false;
		}

		const char* const source = m_translation.GLSL.c_str();
		const GLuint program = glCreateShaderProgramv(ToGLShaderType(m_stage), 1, &source);
		GLint isLinked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked != GL_TRUE)
		{
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
			std::string log(static_cast<size_t>(std::max(logLength, 1)), '\0');
			glGetProgramInfoLog(program, logLength, nullptr, log.data());
			RZE_LOG_ERROR("Couldn't compile shader [" << filePath.GetRelativePath() << "] as GLSL:\n" << TrimEnd(log.c_str()));
			glDeleteProgram(program);
			return false;
		}

		m_program = program;
		return true;
	}

	void GLShaderProgram::Release()
	{
		if (m_program != 0)
		{
			m_device.DeleteLater(GLDevice::EObjectType::Program, m_program);
			m_program = 0;
		}
	}

	GLVertexShader::GLVertexShader(GLDevice& device)
		: m_device(device)
		, m_program(device, GLShaderTranslator::EStage::Vertex)
	{
	}

	void GLVertexShader::Create(const Filepath& filePath, const ShaderInputLayout& inputLayout)
	{
		if (!m_program.Create(filePath))
		{
			return;
		}

		// Each element where the shader reads its semantic. D3D matches the input layout to the shader by semantic too.
		const std::vector<GLShaderTranslator::Varying>& inputs = m_program.GetTranslation().Inputs;
		for (size_t index = 0; index < inputLayout.GetDataCount(); ++index)
		{
			const ShaderInputLayout::ShaderInputLayoutData& element = inputLayout[index];
			const auto input = std::find_if(inputs.begin(), inputs.end(), [&element](const GLShaderTranslator::Varying& varying)
				{
					return varying.Semantic == element.Name;
				});
			// The shader may not read every element, as with D3D
			if (input == inputs.end())
			{
				continue;
			}

			Attribute attribute;
			attribute.Location = input->Location;
			attribute.Format = element.DataFormat;
			attribute.Offset = static_cast<U32>(element.Offset);
			attribute.IsPerInstance = element.DataClassification == EDataClassification::PER_INSTANCE;
			m_attributes.push_back(attribute);
		}
	}

	void GLVertexShader::Release()
	{
		m_program.Release();
		m_attributes.clear();
	}

	void GLVertexShader::SetActive()
	{
		m_device.SetVertexShader(*this);
	}

	void GLVertexShader::SetInputLayout()
	{
		m_device.SetInputLayout(*this);
	}

	GLPixelShader::GLPixelShader(GLDevice& device)
		: m_device(device)
		, m_program(device, GLShaderTranslator::EStage::Pixel)
	{
	}

	void GLPixelShader::Create(const Filepath& filePath)
	{
		m_program.Create(filePath);
	}

	void GLPixelShader::Release()
	{
		m_program.Release();
	}

	void GLPixelShader::SetActive()
	{
		m_device.SetPixelShader(*this);
	}

	GLComputeShader::GLComputeShader(GLDevice& device)
		: m_device(device)
		, m_program(device, GLShaderTranslator::EStage::Compute)
	{
	}

	void GLComputeShader::Create(const Filepath& filePath)
	{
		m_program.Create(filePath);
	}

	void GLComputeShader::Release()
	{
		m_program.Release();
	}

	void GLComputeShader::SetActive()
	{
		m_device.SetComputeShader(*this);
	}
}
