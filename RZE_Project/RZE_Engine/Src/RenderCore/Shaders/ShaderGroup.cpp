#include "StdAfx.h"

#include "RenderCore/HardwareInterface/OpenGL.h"

#include "RenderCore/Shaders/ShaderGroup.h"


GFXShaderGroup::GFXShaderGroup(const std::string& groupName)
	: mGroupName(groupName)
{
}

GFXShaderGroup::~GFXShaderGroup()
{
}

bool GFXShaderGroup::AddShader(U32 shaderType, GFXShader* const shader)
{
	if (!mShaders[shaderType])
	{
		mShaders[shaderType] = shader;
		++mNumShaders;
		return true;
	}
	else 
	{
		LOG_CONSOLE_ARGS("ShaderType %d already exists in group %s. Remove first.", shaderType, mGroupName);
		return false;
	}
}

bool GFXShaderGroup::AddUniform(const std::string& uniformName)
{
	bool bUniformExists = (mUniformMap.count(uniformName) > 0);
	if (!bUniformExists)
	{
		mUniformMap[uniformName] = -1;
		return true;
	}
	else
	{
		LOG_CONSOLE_ARGS("Attempt to add uniform %s to shader group %s failed. Uniform already exists.", uniformName, mGroupName);
		return false;
	}
}

bool GFXShaderGroup::GenerateShaderProgram()
{
	// @todo pretty sure mNumShaders is not doing what I'm expecting here.. should be at least a specific SET of shaders?
	if (!mShaderProgramID && mNumShaders >= 2)
	{
		OpenGLRHI::Get().CreateShaderProgram(mShaderProgramID);
		AttachShaders();
		OpenGLRHI::Get().LinkShaderProgram(mShaderProgramID);
		LoadUniformValues();

		int programLinkStatus = 0;
		OpenGLRHI::Get().GetProgramiv(mShaderProgramID, EGLShaderProgramStatusParam::LinkStatus, &programLinkStatus);
		if (!programLinkStatus)
		{
			LOG_CONSOLE_ARGS("Error linking shaders in shader group %s", mGroupName);
			return false;
		}

		return true;
	}

	return false;
}

void GFXShaderGroup::AttachShaders() const
{
	for (U32 shaderIdx = 0; shaderIdx < EShaderIndex::COUNT; ++shaderIdx)
	{
		if (mShaders[shaderIdx])
		{
			GFXShader* const shader = mShaders[shaderIdx];
			OpenGLRHI::Get().AttachShader(mShaderProgramID, shader->GetShaderID());
		}
	}
}

void GFXShaderGroup::LoadUniformValues()
{
	for (auto& dataPair : mUniformMap)
	{
		std::string key = dataPair.first;

		int uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(mShaderProgramID, key.c_str(), uniformLocation);
		if (uniformLocation < 0)
		{
			continue;
		}

		mUniformMap[key] = uniformLocation;
	}
}