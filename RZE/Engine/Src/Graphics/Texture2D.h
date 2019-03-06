#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Interfaces/Resource.h>

namespace ETextureType
{
	enum T : U32
	{
		Diffuse,
		Specular,
		Normal
	};
}

class Texture2D : public IResource
{
public:
	static FilePath kDefaultDiffuseTexturePath;
	static FilePath kDefaultSpecularTexturePath;
	static FilePath kDefaultNormalTexturePath;

public:
	Texture2D() = delete;
	Texture2D(ETextureType::T textureType);
	virtual ~Texture2D();

public:
	virtual bool Load(const FilePath& filePath) final override;
	virtual void Release() final override;

public:
	U8* GetRawData() const { return mData; }
	ETextureType::T GetTextureType() const;
	Vector2D GetDimensions() const;

	U32 GetTextureBufferID() const;

private:
	U8* mData;

	ETextureType::T mTextureType;

	Int32 mWidth;
	Int32 mHeight;
	Int32 mChannels;

	U32 mGPUBuffer;
};