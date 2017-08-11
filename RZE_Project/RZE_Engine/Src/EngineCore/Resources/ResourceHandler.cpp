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

ResourceHandle ResourceHandler::GetEmptyResourceHandle()
{
    return ResourceHandle("", nullptr);
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
        LOG_CONSOLE_ARGS("Attempting to release resource with key [%s] but it does not currently exist.", resourceHandle.GetID().c_str());
    }
}

ResourceHandle::ResourceHandle()
{
    mResourceSource = nullptr;
}

ResourceHandle::ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource)
{
    mResourceID = resourceID;
    mResourceSource = resourceSource;
}

ResourceHandle::ResourceHandle(const ResourceHandle& rhs)
{
    mResourceID = rhs.mResourceID;
    mResourceSource = rhs.mResourceSource;

    mResourceSource->IncreaseRefCount();
}

ResourceHandle::~ResourceHandle()
{
    if (mResourceSource)
    {
        mResourceSource->DecreaseRefCount();
    }
}

bool ResourceHandle::IsValid() const
{
    return !mResourceID.empty() && mResourceSource->IsValid();
}

const std::string& ResourceHandle::GetID() const
{
    return mResourceID;
}

void ResourceHandle::operator=(const ResourceHandle& rhs)
{
    mResourceID = rhs.mResourceID;
    mResourceSource = rhs.mResourceSource;

    mResourceSource->IncreaseRefCount();
}

bool ResourceHandle::operator==(const ResourceHandle& rhs)
{
    return rhs.mResourceID == mResourceID;
}
