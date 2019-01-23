#include <Diotima/Graphics/GFXTexture2D.h>

#include <Diotima/Driver/OpenGL/OpenGL.h>

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
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		// #NOTE possibly generic texture class where the defining properties are the GL capability?
		openGL.GenerateTexture(1, &mTextureID);
		openGL.BindTexture(EGLCapability::Texture2D, mTextureID);

		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// #TODO enum in OpenGL.h for GL_RGB etc
		openGL.TextureImage2D(EGLCapability::Texture2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, EGLDataType::UnsignedByte, data);
		openGL.BindTexture(EGLCapability::Texture2D, 0);
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
