#include <StdAfx.h>
#include <Graphics/Shader.h>

#include <Graphics/RenderEngine.h>

#include <Graphics/Material.h>

#include <Rendering/Renderer.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Memory/MemoryUtils.h>

bool VertexShader::Load(const Filepath& filePath)
{
	m_shader = Rendering::Renderer::CreateVertexShader(filePath, m_shaderInputLayout);
	m_cameraDataBuf = Rendering::Renderer::CreateConstantBuffer(nullptr, MemoryUtils::AlignSize(sizeof(RenderCamera), 128), 1);
	m_worldMatrixBuf = Rendering::Renderer::CreateConstantBuffer(nullptr, MemoryUtils::AlignSize(sizeof(Matrix4x4), 128), 1);

	return m_shader.IsValid();
}

void VertexShader::Release()
{
	Rendering::Renderer::ReleaseVertexShader(m_shader);
}

bool PixelShader::Load(const Filepath& filePath)
{
	m_shader = Rendering::Renderer::CreatePixelShader(filePath);

	return m_shader.IsValid();
}

void PixelShader::Release()
{
	Rendering::Renderer::ReleasePixelShader(m_shader);
}

const Rendering::PixelShaderHandle PixelShader::GetPlatformObject() const
{
	return m_shader;
}

ShaderTechnique::ShaderTechnique(const std::string& name)
	: m_name(name)
{
}

bool ShaderTechnique::Load(const Filepath& filePath)
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
