#pragma once

#include <Rendering/ShaderHandle.h>

#include <Utils/Interfaces/Resource.h>

#include <string>

class VertexShader : public IResource
{
public:
	VertexShader() = delete;
	VertexShader(const std::string& name)
		: m_name(name) {}

	virtual ~VertexShader() = default;

	bool Load(const FilePath& filePath) final;
	void Release() final;

	const std::string& GetName() const { return m_name; }
	
private:
	const std::string m_name;
	Rendering::VertexShaderHandle m_shader;

};

class PixelShader : public IResource
{
public:
	PixelShader() = delete;
	PixelShader(const std::string& name)
		: m_name(name) {}

	virtual ~PixelShader() = default;

	bool Load(const FilePath& filePath) final;
	void Release() final;

	const std::string& GetName() const { return m_name; }
	
private:
	const std::string m_name;
	Rendering::PixelShaderHandle m_shader;
};


// @TODO
// IResource should be removed, this class should be an aggregate
// of a shader pipeline that describes a certain technique composed of
// n-many shader types or is some sort of resource that is compiled to be aggregate
class ShaderTechnique : public IResource
{
public:
	ShaderTechnique() = delete;
	ShaderTechnique(const std::string& name);
	virtual ~ShaderTechnique() = default;

	bool Load(const FilePath& filePath) final;
	void Release() final;
	
	const std::string& GetName() const { return m_name; }

private:
	const std::string m_name;
	// Inputs?
};