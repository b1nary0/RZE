#include <Diotima/Shaders/ShaderPipeline.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Diotima
{
	GFXShaderPipeline::GFXShaderPipeline(const std::string& groupName)
		: mGroupName(groupName)
		, mNumShaders(0)
		, mShaderProgramID(0)
	{
		for (int shaderIdx = 0; shaderIdx < EShaderIndex::COUNT; ++shaderIdx)
		{
			mShaders[shaderIdx] = 0;
		}
	}

	GFXShaderPipeline::~GFXShaderPipeline()
	{
	}

	bool GFXShaderPipeline::AddShader(EShaderIndex::T shaderType, GFXShader* const shader)
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

	void GFXShaderPipeline::SetUniformMatrix4x4(const char* uniformName, const Matrix4x4& mat)
	{
	}

	void GFXShaderPipeline::SetUniformVector3D(const char* uniformName, const Vector3D& vec)
	{
	}

	void GFXShaderPipeline::SetUniformVector4D(const char* uniformName, const Vector4D& vec)
	{
	}

	void GFXShaderPipeline::SetUniformInt(const char* uniformName, int value)
	{
	}

	void GFXShaderPipeline::SetUniformFloat(const char* uniformName, float value)
	{
	}

	bool GFXShaderPipeline::GenerateShaderProgram()
	{
		return true;
	}

	U32 GFXShaderPipeline::GetShaderProgramID() const
	{
		return mShaderProgramID;
	}

	void GFXShaderPipeline::Use()
	{
	}

	void GFXShaderPipeline::AttachShaders() const
	{
	}
}
