#include "StdAfx.h"

#include "RenderCore/HardwareInterface/OpenGL.h"

#include "RenderCore/Shaders/Shader.h"

GFXShader::GFXShader(const U32 shaderType, const std::string& shaderName, const std::string& sourceCode)
	: mShaderType(shaderType)
	, mShaderName(shaderName)
	, mSourceCode(sourceCode)
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

U32 GFXShader::GetShaderID() const
{
	return mShaderID;
}

U32 GFXShader::GetShaderType() const
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
	if (!mShaderID)
	{
		AssertExpr(GetShaderType() != GFXShaderType::Invalid);
		OpenGLRHI::Get().CreateShader(GetShaderType(), mShaderID);
	}
}

bool GFXShader::Compile()
{
	if (!bIsCompiled)
	{
		OpenGLRHI& openGL = OpenGLRHI::Get();
		openGL.SetShaderSource(GetShaderID(), 1, GetSourceCode().c_str(), nullptr);
		openGL.CompileShader(GetShaderID());

		int CompileResult;
		openGL.GetShaderiv(GetShaderID(), EGLShaderStatusParam::CompileStatus, &CompileResult);

		if (!CompileResult)
		{
			LOG_CONSOLE_ARGS("GFXShader::Compile - Error compiling [%s]", GetShaderName());
		}
		else
		{
			bIsCompiled = true;
		}
	}

	return bIsCompiled;
}
