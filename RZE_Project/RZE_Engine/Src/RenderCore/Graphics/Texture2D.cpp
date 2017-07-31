#include <StdAfx.h>

#include <RenderCore/Graphics/Texture2D.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <STB/stb_image.h>

GFXTexture2D::GFXTexture2D()
    : IResource()
    , mTextureID(0)
{
}

GFXTexture2D::~GFXTexture2D()
{

}

bool GFXTexture2D::Load(const std::string& filePath)
{
    U8* data = stbi_load(filePath.c_str(), &mWidth, &mHeight, &mChannels, 0);
    AssertNotNull(data);

    // #TODO make the OpenGLRHI equivalents
    // #NOTE possibly generic texture class where the defining properties are the GL capability?
    glGenTextures(1, &mTextureID);
    glBindTexture(EGLCapability::Texture2D, mTextureID);
    // #TODO enum in OpenGL.h for GL_RGB etc
    glTexImage2D(EGLCapability::Texture2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, EGLDataType::UnsignedByte, data);
    glGenerateMipmap(EGLCapability::Texture2D);

    stbi_image_free(data);

    return true;
}
