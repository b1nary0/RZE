#include <StdAfx.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

bool VertexShader::Load(const FilePath& filePath)
{
	m_shader = Rendering::Renderer::CreateVertexShader(filePath);
	return m_shader.IsValid();
}

void VertexShader::Release()
{
	Rendering::Renderer::ReleaseVertexShader(m_shader);
}

bool PixelShader::Load(const FilePath& filePath)
{
	m_shader = Rendering::Renderer::CreatePixelShader(filePath);
	return m_shader.IsValid();
}

void PixelShader::Release()
{
	Rendering::Renderer::ReleasePixelShader(m_shader);
}

ShaderTechnique::ShaderTechnique(const std::string& name)
	: m_name(name)
{
}

bool ShaderTechnique::Load(const FilePath& filePath)
{
	if (filePath.IsValid())
	{
		return true;
	}

	return false;
}

void ShaderTechnique::Release()
{
	// #TODO
	// Write this function le-mao
}
