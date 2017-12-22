#pragma once

#include <string>
#include <unordered_map>

#include <Diotima/Shaders/Shader.h>

#include <Utils/PrimitiveDefs.h>

class Matrix4x4;
class Vector3D;
class Vector4D;

namespace Diotima
{
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

		void SetUniformInt(const char* uniformName, int value);
		void SetUniformFloat(const char* uniformName, float value);
		void SetUniformVector3D(const char* uniformName, const Vector3D& vec);
		void SetUniformVector4D(const char* uniformName, const Vector4D& vec);
		void SetUniformMatrix4x4(const char* uniformName, const Matrix4x4& mat);

		bool GenerateShaderProgram();

		U32 GetShaderProgramID() const;

		void Use();

	private:
		void AttachShaders() const;

		U32 mNumShaders;
		U32 mShaderProgramID;

		std::string mGroupName;
		GFXShader* mShaders[EShaderIndex::COUNT];
	};
}
