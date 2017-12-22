#include <Diotima/Graphics/Texture2D.h>

#include <STB/stb_image.cpp>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	GFXTexture2D::GFXTexture2D(ETextureType::T textureType/* = ETextureType::Diffuse*/)
		: IResource()
		, mTextureID(0)
		, mTextureType(textureType)
	{
	}

	GFXTexture2D::~GFXTexture2D()
	{

	}

	bool GFXTexture2D::Load(const std::string& filePath)
	{
		// #TODO(Josh) This will need to be customized for different bit sizes... 24, 32 etc?
		U8* data = stbi_load(filePath.c_str(), &mWidth, &mHeight, &mChannels, STBI_rgb);
		AssertNotNull(data);

		OpenGLRHI& openGL = OpenGLRHI::Get();

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

		// #TODO(Josh) deal with later
		//glGenerateMipmap(EGLCapability::Texture2D);

		stbi_image_free(data);

		return true;
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
