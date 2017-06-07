#pragma once

namespace EGFXShaderType
{
	enum Value : U32
	{
		Vertex,
		Fragment,
		Geometry,
		Tessellation,
		Compute,
		Invalid = UINT_MAX
	};
}

class GFXShader
{
public:
	GFXShader() = delete;
	GFXShader(const U32 shaderType, const std::string& shaderName, const std::string& sourceCode);
	~GFXShader();

	U32 GetShaderID() const;
	U32 GetShaderType() const;
	const std::string& GetShaderName();
	const std::string& GetSourceCode();

	void Create();
	bool Compile();

private:
	bool bIsCompiled;
	bool bIsCreated;

	U32 mShaderType;
	U32 mShaderID;
	std::string mShaderName;
	std::string mSourceCode;
};
