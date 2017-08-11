#include <StdAfx.h>

#include <RenderCore/Graphics/Font/FontHandler.h>

FontHandler::FontHandler()
{
    if (FT_Init_FreeType(&mFreeType) != 0)
    {
        LOG_CONSOLE("Issue loading FreeType.");
    }
}

FontHandler::~FontHandler()
{

}

bool FontHandler::LoadFont(const std::string& name, const std::string& filePath)
{
    ResourceHandle fontHandle = RZE_Engine::Get()->GetResourceHandler().RequestResource<GFXFont>(filePath, name, this);
    if (fontHandle.IsValid())
    {
        mFonts[name] = fontHandle;

        return true;
    }

    return false;
}

ResourceHandle FontHandler::GetFont(const std::string& name)
{
    auto iter = mFonts.find(name);
    if (iter != mFonts.end())
    {
        return (*iter).second;
    }
    
    return ResourceHandle::EmptyHandle();
}

FT_Library& FontHandler::GetLibrary()
{
    return mFreeType;
}
