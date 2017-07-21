#include <StdAfx.h>
#include <EngineCore/Resources/ResourceHandler.h>

ResourceHandler::ResourceHandler()
{
}

ResourceHandler::~ResourceHandler()
{
}

ResourceHandle::ResourceHandle(const std::string& resourceID)
{
    mResourceID = resourceID;
}

bool ResourceHandle::IsValid()
{
    return !mResourceID.empty();
}

const std::string& ResourceHandle::GetID() const
{
    return mResourceID;
}
