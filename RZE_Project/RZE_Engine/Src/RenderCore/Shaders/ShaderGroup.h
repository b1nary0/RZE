#pragma once

#include <string>
#include <unordered_map>

#include "RenderCore/Shaders/Shader.h"

#include "Utils/PrimitiveDefs.h"

class GFXShaderGroup
{
	typedef std::unordered_map<std::string, Int32> UniformMap;

public:
	GFXShaderGroup(const std::string& groupName);
	~GFXShaderGroup();
	
	struct EShaderIndex
	{
		enum Value : U32
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
	
	bool AddShader(U32 shaderType, GFXShader* const shader);
	bool AddUniform(const std::string& uniformName);
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
