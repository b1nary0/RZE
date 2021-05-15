#pragma once

#include <Utils/Interfaces/Resource.h>

#include <string>

class ShaderTechnique : public IResource
{
public:
	ShaderTechnique() = delete;
	ShaderTechnique(const std::string& name);
	virtual ~ShaderTechnique() = default;

	virtual bool Load(const FilePath& filePath) final override;
	virtual void Release() final override;

	inline Int32 GetHardwareID() const { return mHWShaderHandle; }

	inline const std::string& GetName() const { return mName; }

private:
	const std::string mName;
	// #TODO 
	// GPU shader reference
	Int32 mHWShaderHandle = -1;
	// Inputs?
};