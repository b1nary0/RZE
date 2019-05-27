#include <Diotima/Graphics/GFXTexture2D.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXTexture2D::GFXTexture2D(U8* data, Int32 width, Int32 height, Int32 channels, ETextureType::T textureType/* = ETextureType::Diffuse*/)
		: mTextureID(0)
		, mTextureType(textureType)
		, mWidth(width)
		, mHeight(height)
		, mChannels(channels)
	{
	}

	GFXTexture2D::~GFXTexture2D()
	{

	}

	U32 GFXTexture2D::GetTextureID()
	{
		return mTextureID;
	}

	ETextureType::T GFXTexture2D::GetTextureType()
	{
		return mTextureType;
	}

	Vector2D GFXTexture2D::GetDimensions()
	{
		return Vector2D(mWidth, mHeight);
	}
}
