#pragma once

#include <Utils/Interfaces/Resource.h>

class ShaderTechnique : public IResource
{
public:
	ShaderTechnique() = default;
	virtual ~ShaderTechnique() = default;

	virtual bool Load(const FilePath& filePath) final override;
	virtual void Release() final override;

	inline Int32 GetHardwareID() const { return mHWShaderHandle; }

private:
	// #TODO 
	// GPU shader reference
	Int32 mHWShaderHandle = -1;
	// Inputs?
};