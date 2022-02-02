#include <StdAfx.h>

#include <RZE.h>

#include <Graphics/Shader.h>
#include <Graphics/RenderEngine.h>

ShaderTechnique::ShaderTechnique(const std::string& name)
	: mName(name)
{
}

bool ShaderTechnique::Load(const FilePath& filePath)
{
	if (filePath.IsValid())
	{
		//mHWShaderHandle = RZE::GetRenderEngine().CreatePixelShader(filePath);
		return true;
	}

	return false;
}

void ShaderTechnique::Release()
{
	// #TODO
	// Write this function le-mao
}
