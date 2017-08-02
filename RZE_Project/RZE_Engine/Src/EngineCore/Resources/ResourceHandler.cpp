#include <StdAfx.h>
#include <EngineCore/Resources/ResourceHandler.h>

ResourceHandler::ResourceHandler()
{
}

ResourceHandler::~ResourceHandler()
{
}

void ResourceHandler::Init()
{
    mAllocator.Init();
}

void ResourceHandler::ReleaseResource(ResourceHandle& resourceHandle)
{
    auto iter = mResourceTable.find(resourceHandle.GetID());
    if (iter != mResourceTable.end())
    {
        ResourceSource& resourceSource = (*iter).second;
        resourceSource.DecreaseRefCount();

        if (!resourceSource.IsReferenced())
        {
            resourceSource.Destroy();
            mResourceTable.erase(iter);
        }
    }
    else
    {
        LOG_CONSOLE_ARGS("Attempting to release resource with key [%s] but it does not currently exist.", resourceHandle.GetID());
    }
}

ResourceHandle::ResourceHandle(const std::string& resourceID)
{
    mResourceID = resourceID;
}

bool ResourceHandle::IsValid() const
{
    return !mResourceID.empty();
}

const std::string& ResourceHandle::GetID() const
{
    return mResourceID;
}
