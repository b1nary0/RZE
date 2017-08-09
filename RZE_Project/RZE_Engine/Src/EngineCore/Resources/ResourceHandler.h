#pragma once

#include <unordered_map>

#include <EngineCore/Resources/Resource.h>
#include <EngineCore/Platform/Memory/BlockAllocator.h>

#include <DebugUtils/Debug.h>
#include <Utils/Conversions.h>


class ResourceHandler
{
    friend class ResourceHandle;

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
        bool IsValid() { return IsReferenced() && mResource; }

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

    void Init();

    template <class ResourceT, class... Args>
    ResourceHandle RequestResource(const std::string& resourcePath, Args... args);

    void ReleaseResource(ResourceHandle& resourceHandle);

    template <class ResourceT>
    ResourceT* GetResource(const ResourceHandle& resourceHandle);

private:
    template <class ResourceT, class... Args>
    IResource* CreateAndLoadResource(const std::string& resourcePath, Args&&... args);

    BlockAllocator mAllocator;
    std::unordered_map<std::string, ResourceSource> mResourceTable;
};

class ResourceHandle
{
    friend class ResourceHandler;

public:
    ResourceHandle();
    ResourceHandle(const ResourceHandle& rhs);
    ~ResourceHandle();

    bool IsValid() const;
    const std::string& GetID() const;

    bool operator==(const ResourceHandle& rhs);
    void operator=(const ResourceHandle& rhs);

private:
    ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource);

    std::string mResourceID;
    ResourceHandler::ResourceSource* mResourceSource;
};

// #TODO probably move this into an inl maybe?
template <class ResourceT, class... Args>
ResourceHandle ResourceHandler::RequestResource(const std::string& resourcePath, Args... args)
{
    std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath);
    LOG_CONSOLE_ARGS("Requesting resource [%s]", resourceKey.c_str());

    auto iter = mResourceTable.find(resourceKey);
    if (iter == mResourceTable.end())
    {
        IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath, args...);
        ResourceSource resourceSource(resource);
        
        mResourceTable[resourceKey] = resourceSource;
        return ResourceHandle(resourceKey, &mResourceTable[resourceKey]);
    }

    ResourceSource& resourceSource = (*iter).second;
    resourceSource.IncreaseRefCount();

    return ResourceHandle(resourceKey, &resourceSource);
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

template <class ResourceT, class... Args>
IResource* ResourceHandler::CreateAndLoadResource(const std::string& resourcePath, Args&&... args)
{
    IResource* resource = new ResourceT(args...);//new (mAllocator.Allocate(sizeof(ResourceT))) ResourceT;
    AssertNotNull(resource);
    resource->Load(resourcePath);

    return resource;
}