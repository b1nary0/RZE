#pragma once

#include <string>

#include <EngineCore/Resources/Resource.h>

#include <Utils/PrimitiveDefs.h>

class GFXTexture2D : public IResource
{
public:
    GFXTexture2D();
    ~GFXTexture2D();

    virtual bool Load(const std::string& filePath);

private:
    U32 mTextureID;

    Int32 mWidth;
    Int32 mHeight;
    Int32 mChannels;
};
