#include <StdAfx.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Conversions.h>

ResourceHandler::ResourceHandler()
{
}

ResourceHandler::~ResourceHandler()
{
}

ResourceHandle ResourceHandler::RequestResource(const std::string& resourcePath)
{
    std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath);
    auto iter = mResourceTable.find(resourceKey);

    return ResourceHandle(resourceKey);
}

template <class ResourceT>
IResource* ResourceHandler::CreateAndLoadResource()
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
