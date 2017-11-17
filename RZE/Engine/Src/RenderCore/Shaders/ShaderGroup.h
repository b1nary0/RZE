#pragma once

#include <string>
#include <unordered_map>

#include "RenderCore/Shaders/Shader.h"

#include "Utils/PrimitiveDefs.h"

class Matrix4x4;
class Vector3D;
class Vector4D;

class GFXShaderGroup
{
	typedef std::unordered_map<std::string, Int32> UniformMap;

public:
	GFXShaderGroup(const std::string& groupName);
	~GFXShaderGroup();

	struct EShaderIndex
	{
		enum T : U32
		{
			Vertex = 0,
			Fragment = 1,
			Compute = 2,
			TessControl = 3,
			TessEval = 4,
			Geometry = 5,
			COUNT
		};
	};

	bool AddShader(EShaderIndex::T shaderType, GFXShader* const shader);
	bool AddUniform(const std::string& uniformName);

	void SetUniformInt(const std::string& uniformName, int value);
	void SetUniformFloat(const std::string& uniformName, float value);
	void SetUniformVector3D(const std::string& uniformName, const Vector3D& vec);
	void SetUniformVector4D(const std::string& uniformName, const Vector4D& vec);
	void SetUniformMatrix4x4(const std::string& uniformName, const Matrix4x4& mat);

	bool GenerateShaderProgram();

	U32 GetShaderProgramID() const;

	void Use();

private:
	void AttachShaders() const;
	void LoadUniformValues();

	U32 mNumShaders;
	U32 mShaderProgramID;

	std::string mGroupName;

	UniformMap mUniformMap;
	GFXShader* mShaders[EShaderIndex::COUNT];
};