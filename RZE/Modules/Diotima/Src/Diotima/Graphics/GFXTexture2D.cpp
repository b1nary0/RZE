#include <Diotima/Graphics/GFXTexture2D.h>

#include <Diotima/Driver/OpenGL/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXTexture2D::GFXTexture2D(ETextureType::T textureType/* = ETextureType::Diffuse*/)
		: mTextureID(0)
		, mTextureType(textureType)
		, mWidth(0)
		, mHeight(0)
		, mChannels(0)
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
