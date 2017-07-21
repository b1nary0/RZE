#pragma once

#include <unordered_map>

#include <EngineCore/Resources/Resource.h>

#include <Utils/Conversions.h>
#include <DebugUtils/Debug.h>

class ResourceHandle
{
    friend class ResourceHandler;


public:
    ResourceHandle() {}

    bool IsValid();
    const std::string& GetID() const;

    bool operator==(const ResourceHandle& rhs)
    {
        rhs.mResourceID == mResourceID;
    }

private:
    ResourceHandle(const std::string& resourceID);

    std::string mResourceID;
};

class ResourceHandler
{
    typedef std::pair<std::string, IResource*> ResourcePair;

public:
    ResourceHandler();
    ~ResourceHandler();

    template <class ResourceT>
    ResourceHandle RequestResource(const std::string& resourcePath);

    template <class ResourceT>
    ResourceT* GetResource(const ResourceHandle& resourceHandle);

private:
    template <class ResourceT>
    IResource* CreateAndLoadResource(const std::string& resourcePath);

    std::unordered_map<std::string, IResource*> mResourceTable;
};

// #TODO probably move this into an inl maybe?
template <class ResourceT>
ResourceHandle ResourceHandler::RequestResource(const std::string& resourcePath)
{
    std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath);
    auto iter = mResourceTable.find(resourceKey);
    if (iter == mResourceTable.end())
    {
        IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath);
        mResourceTable.insert(ResourcePair(resourceKey, resource));

        return ResourceHandle(resourceKey);
    }

    return ResourceHandle(resourceKey);
}

template <class ResourceT>
ResourceT* ResourceHandler::GetResource(const ResourceHandle& resourceHandle)
{
    auto iter = mResourceTable.find(resourceHandle.GetID());
    if (iter != mResourceTable.end())
    {
        return static_cast<ResourceT*>(mResourceTable.at(resourceHandle.GetID()));
    }

    LOG_CONSOLE_ARGS("Resource with key [%s] does not exist. Request it first.", resourceHandle.GetID().c_str());
    return nullptr;
}

template <class ResourceT>
IResource* ResourceHandler::CreateAndLoadResource(const std::string& resourcePath)
{
    IResource* resource = new ResourceT();
    AssertNotNull(resource);
    resource->Load(resourcePath);

    return resource;
}