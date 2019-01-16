#include <StdAfx.h>
#include <Graphics/Texture2D.h>

#include <STB/stb_image.cpp>

Texture2D::Texture2D(ETextureType::T textureType)
	: IResource()
	, mData(nullptr)
	, mTextureType(textureType)
	, mWidth(0)
	, mHeight(0)
	, mChannels(0)
{
}

Texture2D::~Texture2D()
{
}

bool Texture2D::Load(const FilePath& filePath)
{
	// #TODO(Josh) This will need to be customized for different bit sizes... 24, 32 etc?
	U8* data = stbi_load(filePath.GetAbsolutePath().c_str(), &mWidth, &mHeight, &mChannels, STBI_rgb);
	if (data == nullptr)
	{
		// Default texture
		data = stbi_load(kDefaultDiffuseTexturePath.GetAbsolutePath().c_str(), &mWidth, &mHeight, &mChannels, STBI_rgb);
		AssertNotNull(data);
	}

	return data != nullptr;
}

void Texture2D::Release()
{
}

ETextureType::T Texture2D::GetTextureType() const
{
	return mTextureType;
}

Vector2D Texture2D::GetDimensions() const
{
	return Vector2D(mWidth, mHeight);
}
