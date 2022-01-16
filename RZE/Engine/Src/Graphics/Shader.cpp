#include <StdAfx.h>

#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>

ShaderTechnique::ShaderTechnique(const std::string& name)
	: mName(name)
{
}

bool ShaderTechnique::Load(const FilePath& filePath)
{
	if (filePath.IsValid())
	{
		mHWShaderHandle = RZE_Application::RZE().GetRenderer().CreatePixelShader(filePath);
		return true;
	}

	return false;
}

void ShaderTechnique::Release()
{
	// #TODO
	// Write this function le-mao
}
