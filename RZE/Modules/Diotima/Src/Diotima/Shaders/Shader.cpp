#include <Diotima/Shaders/Shader.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>

namespace Diotima
{
	GFXShader::GFXShader(const U32 shaderType, const std::string& shaderName)
		: mShaderName(shaderName)
		, mShaderID(0)
		, bIsCreated(false)
		, bIsCompiled(false)
	{
	}

	GFXShader::~GFXShader()
	{
	}

	bool GFXShader::Load(const FilePath& filePath)
	{
		File shaderFile(filePath.GetAbsolutePath());
		mSourceCode = shaderFile.Content();

		return !shaderFile.Empty();
	}

	void GFXShader::Release()
	{

	}

	U32 GFXShader::GetShaderID() const
	{
		return mShaderID;
	}

	const std::string& GFXShader::GetShaderName()
	{
		return mShaderName;
	}

	const std::string& GFXShader::GetSourceCode()
	{
		return mSourceCode;
	}

	void GFXShader::Create()
	{
		if (!bIsCreated)
		{
			//@todo:josh this may need better handling here... can CreateShader fail?
			bIsCreated = true;
		}
	}

	bool GFXShader::Compile()
	{
		if (!bIsCompiled)
		{

		}

		return bIsCompiled;
	}
}
