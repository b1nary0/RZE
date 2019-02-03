#pragma once

#include <string>

#include <Utils/Interfaces/Resource.h>
#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	namespace EGFXShaderIndex
	{
		enum T : U32
		{
			Vertex,
			Fragment,
			Geometry,
			Tessellation,
			Compute,
			Invalid = UINT_MAX
		};
	}

	class GFXShader : public IResource
	{
	public:
		GFXShader() = delete;
		GFXShader(const U32 shaderType, const std::string& shaderName);
		~GFXShader();

		virtual bool Load(const FilePath& filePath) override;
		virtual void Release() override;

		U32 GetShaderID() const;
		const std::string& GetShaderName();
		const std::string& GetSourceCode();

		void Create();
		bool Compile();

	private:
		bool bIsCompiled;
		bool bIsCreated;

		U32 mShaderID;
		std::string mShaderName;
		std::string mSourceCode;
	};
}
