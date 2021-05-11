#include <StdAfx.h>

#include <Graphics/Shader.h>

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
