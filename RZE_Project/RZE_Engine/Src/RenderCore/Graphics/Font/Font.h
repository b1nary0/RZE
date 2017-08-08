#pragma once

#include <string>

#include <EngineCore/Resources/Resource.h>

class FontHandler;

struct CharacterDescription
{
    U32 mTextureID;
    U32 mAdvance;
    Vector2D mSize;
    Vector2D mBearing;
};

class GFXFont : public IResource
{
public:
    GFXFont(const std::string& name, FontHandler* fontHandler);
    ~GFXFont();

    virtual bool            Load(const std::string& filePath) override;

    void                    SetSize(int size);

private:
    void                    InternalSetSize(int size);

    // This function is naive and only currently supports 0 - 127 chars
    void                    LoadCharacters();

private:
    int mSize;

    FontHandler*            mFontHandler;
    FT_Face                 mFace;

    std::unordered_map<U8, CharacterDescription> mCharacters;
};
