#pragma once

class GFXShader
{
public:
	GFXShader();
	~GFXShader();

	U32 GetShaderID() const;

private:
	U32 mShaderID;
};
