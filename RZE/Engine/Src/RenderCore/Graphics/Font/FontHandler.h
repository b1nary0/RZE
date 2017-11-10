#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>

#include <RenderCore/Graphics/Font/Font.h>

#include <EngineCore/Resources/ResourceHandler.h>

class FontHandler
{
public:
	FontHandler();
	~FontHandler();

	bool LoadFont(const std::string& name, const std::string& filePath);

	ResourceHandle GetFont(const std::string& name);

	FT_Library& GetLibrary();

private:
	FT_Library mFreeType;

	std::unordered_map<std::string, ResourceHandle> mFonts;
};