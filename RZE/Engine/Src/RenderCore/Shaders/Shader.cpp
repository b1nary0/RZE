#include "StdAfx.h"

#include "RenderCore/HardwareInterface/OpenGL.h"

#include "RenderCore/Shaders/Shader.h"

GFXShader::GFXShader(const EGLShaderType::T shaderType, const std::string& shaderName)
	: mShaderType(shaderType)
	, mShaderName(shaderName)
	, mShaderID(0)
	, bIsCreated(false)
	, bIsCompiled(false)
{
}

GFXShader::~GFXShader()
{
	U8 bIsValidShader = 0;
	OpenGLRHI::Get().IsShader(GetShaderID(), bIsValidShader);
	if (bIsValidShader)
	{
		OpenGLRHI::Get().DeleteShader(GetShaderID());
	}
}

bool GFXShader::Load(const std::string& filePath)
{
	File shaderFile(filePath);
	mSourceCode = shaderFile.Content();

	return !shaderFile.Empty();
}

U32 GFXShader::GetShaderID() const
{
	return mShaderID;
}

EGLShaderType::T GFXShader::GetShaderType() const
{
	return mShaderType;
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
		AssertExpr(GetShaderType() != EGLShaderType::Invalid);
		OpenGLRHI::Get().CreateShader(GetShaderType(), mShaderID);

		//@todo:josh this may need better handling here... can CreateShader fail?
		bIsCreated = true;
	}
}

bool GFXShader::Compile()
{
	if (!bIsCompiled)
	{
		OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.SetShaderSource(GetShaderID(), 1, GetSourceCode().c_str(), nullptr);
		openGL.CompileShader(GetShaderID());

		int compileResult = -1;
		openGL.GetShaderiv(GetShaderID(), EGLShaderStatusParam::CompileStatus, &compileResult);

		if (compileResult == EGLBooleanValue::False)
		{
			LOG_CONSOLE_ARGS("GFXShader::Compile - Error compiling [%s]", GetShaderName().c_str());
			openGL.LogShaderInfo(mShaderID);
		}
		else
		{
			bIsCompiled = true;
		}
	}

	return bIsCompiled;
}
