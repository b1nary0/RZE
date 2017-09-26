#pragma once

#include <EngineCore/Resources/Resource.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

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
	GFXShader(const EGLShaderType::T shaderType, const std::string& shaderName);
	~GFXShader();

	virtual bool Load(const std::string& filePath) override;

	U32 GetShaderID() const;
	EGLShaderType::T GetShaderType() const;
	const std::string& GetShaderName();
	const std::string& GetSourceCode();

	void Create();
	bool Compile();

private:
	bool bIsCompiled;
	bool bIsCreated;

	U32 mShaderID;
	EGLShaderType::T mShaderType;
	std::string mShaderName;
	std::string mSourceCode;
};
