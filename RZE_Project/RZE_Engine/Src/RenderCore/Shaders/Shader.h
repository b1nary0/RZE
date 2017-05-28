#pragma once

namespace GFXShaderType
{
	enum Value : U32
	{
		Invalid,
		Vertex,
		Fragment,
		Geometry,
		Tessellation,
		Compute
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

	U32 mShaderType;
	std::string mShaderName;
	std::string mSourceCode;
	U32 mShaderID;
};
