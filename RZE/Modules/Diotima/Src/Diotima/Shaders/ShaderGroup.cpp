#include <Diotima/Shaders/ShaderGroup.h>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Diotima
{
	GFXShaderGroup::GFXShaderGroup(const std::string& groupName)
		: mGroupName(groupName)
		, mNumShaders(0)
		, mShaderProgramID(0)
	{
		for (int shaderIdx = 0; shaderIdx < EShaderIndex::COUNT; ++shaderIdx)
		{
			mShaders[shaderIdx] = 0;
		}
	}

	GFXShaderGroup::~GFXShaderGroup()
	{
	}

	bool GFXShaderGroup::AddShader(EShaderIndex::T shaderType, GFXShader* const shader)
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

	void GFXShaderGroup::SetUniformMatrix4x4(const char* uniformName, const Matrix4x4& mat)
	{
		Int32 uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(GetShaderProgramID(), uniformName, uniformLocation);
		if (uniformLocation >= 0)
		{
			OpenGLRHI::Get().SetUniformMat4x4(uniformLocation, 1, EGLBooleanValue::False, mat.GetValuePtr());
		}
	}

	void GFXShaderGroup::SetUniformVector3D(const char* uniformName, const Vector3D& vec)
	{
		Int32 uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(GetShaderProgramID(), uniformName, uniformLocation);
		if (uniformLocation >= 0)
		{
			OpenGLRHI::Get().SetUniformVec3D(uniformLocation, vec.X(), vec.Y(), vec.Z());
		}
	}

	void GFXShaderGroup::SetUniformVector4D(const char* uniformName, const Vector4D& vec)
	{
		Int32 uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(GetShaderProgramID(), uniformName, uniformLocation);
		if (uniformLocation >= 0)
		{
			OpenGLRHI::Get().SetUniformVec4D(uniformLocation, vec.X(), vec.Y(), vec.Z(), vec.W());
		}
	}

	void GFXShaderGroup::SetUniformInt(const char* uniformName, int value)
	{
		Int32 uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(GetShaderProgramID(), uniformName, uniformLocation);
		if (uniformLocation >= 0)
		{
			OpenGLRHI::Get().SetUniformInt(uniformLocation, value);
		}
	}

	void GFXShaderGroup::SetUniformFloat(const char* uniformName, float value)
	{
		Int32 uniformLocation = -1;
		OpenGLRHI::Get().UniformLocation(GetShaderProgramID(), uniformName, uniformLocation);
		if (uniformLocation >= 0)
		{
			OpenGLRHI::Get().SetUniformFloat(uniformLocation, value);
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

			int programLinkStatus = 0;
			OpenGLRHI::Get().GetProgramiv(mShaderProgramID, EGLShaderProgramStatusParam::LinkStatus, &programLinkStatus);
			if (!programLinkStatus)
			{
				int kLogLength = 0;
				OpenGLRHI::Get().GetProgramiv(mShaderProgramID, EGLShaderProgramStatusParam::InfoLogLength, &kLogLength);
				std::vector<char> errorLog(kLogLength);

				int length;
				OpenGLRHI::Get().GetProgramInfoLog(GetShaderProgramID(), kLogLength, &length, &errorLog[0]);

				LOG_CONSOLE_ARGS("Error linking shaders in shader group [%s]", mGroupName.c_str());
				LOG_CONSOLE_ARGS("Error: %s", std::string(errorLog.data()).c_str());
				return false;
			}

			return true;
		}

		return false;
	}

	U32 GFXShaderGroup::GetShaderProgramID() const
	{
		return mShaderProgramID;
	}

	void GFXShaderGroup::Use()
	{
		U8 bIsShaderProgram = false;
		OpenGLRHI::Get().IsShaderProgram(GetShaderProgramID(), bIsShaderProgram);

		if (bIsShaderProgram)
		{
			OpenGLRHI::Get().UseShaderProgram(GetShaderProgramID());
		}
		else
		{
			LOG_CONSOLE_ARGS("Shader group [%s] has not been generated.", mGroupName.c_str());
		}
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
}
