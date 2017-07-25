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
private:
    class ResourceSource
    {
    public:
        ResourceSource()
        {
            mReferenceCount = 0;
            mResource = nullptr;
        }

        ResourceSource(IResource* resource)
        {
            mReferenceCount = 1;
            mResource = resource;
        }

        ResourceSource(ResourceSource& rhs)
        {
            rhs.IncreaseRefCount();
            mReferenceCount = rhs.mReferenceCount;
            mResource = rhs.mResource;
        }

        void IncreaseRefCount() { ++mReferenceCount; }
        void DecreaseRefCount() { --mReferenceCount; }

        IResource* GetResource() { return mResource; }

        bool IsReferenced() { return mReferenceCount != 0; }

        void Destroy()
        {
            AssertNotNull(mResource);
            AssertExpr(mReferenceCount == 0);

            delete mResource;
            mResource = nullptr;
        }

    private:
        int mReferenceCount;
        IResource* mResource;
    };

    typedef std::pair<std::string, ResourceSource> ResourcePair;


public:
    ResourceHandler();
    ~ResourceHandler();

    template <class ResourceT>
    ResourceHandle RequestResource(const std::string& resourcePath);

    void ReleaseResource(ResourceHandle& resourceHandle);

    template <class ResourceT>
    ResourceT* GetResource(const ResourceHandle& resourceHandle);

private:
    template <class ResourceT>
    IResource* CreateAndLoadResource(const std::string& resourcePath);

    std::unordered_map<std::string, ResourceSource> mResourceTable;
};

// #TODO probably move this into an inl maybe?
template <class ResourceT>
ResourceHandle ResourceHandler::RequestResource(const std::string& resourcePath)
{
    std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath);
    LOG_CONSOLE_ARGS("Requesting resource [%s]", resourceKey.c_str());

    auto iter = mResourceTable.find(resourceKey);
    if (iter == mResourceTable.end())
    {
        IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath);
        ResourceSource resourceSource(resource);
        
        mResourceTable[resourceKey] = resourceSource;
        return ResourceHandle(resourceKey);
    }

    ResourceSource& resourceSource = (*iter).second;
    resourceSource.IncreaseRefCount();

    return ResourceHandle(resourceKey);
}

template <class ResourceT>
ResourceT* ResourceHandler::GetResource(const ResourceHandle& resourceHandle)
{
    auto iter = mResourceTable.find(resourceHandle.GetID());
    if (iter != mResourceTable.end())
    {
        ResourceSource& resourceSource = (*iter).second;
        return static_cast<ResourceT*>(resourceSource.GetResource());
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