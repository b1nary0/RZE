#pragma once

#include <string>

#include <EngineCore/Resources/Resource.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

class FontHandler;

struct CharacterDescription
{
    CharacterDescription()
    {
        // Temporary for first pass
        mVAO.Init();
        mVAO.Bind();

        mVBO.Init();
        mVBO.Bind();

        mVBO.SetBufferData(nullptr, sizeof(GLfloat) * 6 * 4);

        OpenGLRHI::Get().EnableVertexAttributeArray(0);
        OpenGLRHI::Get().VertexAttribPointer(0, 4, EGLDataType::Float, EGLBooleanValue::False, 4 * sizeof(GLfloat), 0);
    }

    U32 mTextureID;
    U32 mAdvance;
    Vector2D mSize;
    Vector2D mBearing;

    OpenGLVAO mVAO;
    OpenGLVBO mVBO;
};

class GFXFont : public IResource
{
public:
    GFXFont(const std::string& name, FontHandler* fontHandler);
    ~GFXFont();

    virtual bool            Load(const std::string& filePath) override;

    void                    SetSize(int size);

	CharacterDescription& GetCharacterDesc(char c);

private:
    void                    InternalSetSize(int size);

    // This function is naive and only currently supports 0 - 127 chars
    void                    LoadCharacters();

private:
    int mSize;
    std::string mName;

    FontHandler*            mFontHandler;
    FT_Face                 mFace;

    std::unordered_map<U8, CharacterDescription> mCharacters;
};
