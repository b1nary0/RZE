#include <StdAfx.h>

#include <RenderCore/Graphics/Font/Font.h>

#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/Graphics/Font/FontHandler.h>

GFXFont::GFXFont(const std::string& name, FontHandler* fontHandler)
{
	mName = name;
	mFontHandler = fontHandler;
}

GFXFont::~GFXFont()
{

}

bool GFXFont::Load(const std::string& filePath)
{
	if (FT_New_Face(mFontHandler->GetLibrary(), filePath.c_str(), 0, &mFace) != 0)
	{
		LOG_CONSOLE_ARGS("Issue loading font at [%s].", filePath.c_str());
		return false;
	}

	LoadCharacters();

	return true;
}

void GFXFont::SetSize(int size)
{
	mSize = size;
	InternalSetSize(mSize);
}

CharacterDescription& GFXFont::GetCharacterDesc(char c)
{
	return mCharacters.at(static_cast<U8>(c));
}

void GFXFont::InternalSetSize(int size)
{
	FT_Set_Pixel_Sizes(mFace, 0, size);
}

void GFXFont::LoadCharacters()
{
	SetSize(12);

	// #TODO(Josh) make opengl rhi call here and also give the enum an entry in opengl.h
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // #NOTE(Josh) if having issues, check this is set back to normal

	for (U8 character = 0; character < 128; character++)
	{
		if (FT_Load_Char(mFace, character, FT_LOAD_RENDER))
		{
			LOG_CONSOLE_ARGS("Could not load glyph for character [%c]", character);
			continue;
		}

		// #TODO(Josh) make this better. Will have to add some functionality to GFXTexture2D to support the width height stuff and buffer.
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			mFace->glyph->bitmap.width,
			mFace->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			mFace->glyph->bitmap.buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width = static_cast<int>(mFace->glyph->bitmap.width);
		int height = static_cast<int>(mFace->glyph->bitmap.rows);

		CharacterDescription charDesc;
		charDesc.mTextureID = texture;
		charDesc.mSize = Vector2D(width, height);
		charDesc.mBearing = Vector2D(mFace->glyph->bitmap_left, mFace->glyph->bitmap_top);
		charDesc.mAdvance = mFace->glyph->advance.x;

		mCharacters[character] = charDesc;
	}

	FT_Done_Face(mFace);
}
