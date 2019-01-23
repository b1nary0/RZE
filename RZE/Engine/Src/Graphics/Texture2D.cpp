#include <StdAfx.h>
#include <Graphics/Texture2D.h>

#include <STB/stb_image.cpp>

FilePath Texture2D::kDefaultDiffuseTexturePath = FilePath("Assets/2D/NOASSET_Texture.png");

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
	mData = stbi_load(filePath.GetAbsolutePath().c_str(), &mWidth, &mHeight, &mChannels, STBI_rgb);
	if (mData == nullptr)
	{
		// Default texture
		mData = stbi_load(kDefaultDiffuseTexturePath.GetAbsolutePath().c_str(), &mWidth, &mHeight, &mChannels, STBI_rgb);
		AssertNotNull(mData);
	}

	return mData != nullptr;
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
