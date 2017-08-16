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
		friend class ResourceHandler;

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
		// Bypasses asserts needed for regular destruction, 
		// as when we shut down there may be a positive reference count, but thats ok
		// when we just want to shut the system down. We will log each issue in the console
		// for reference when we call ShutDown() on ResourceHandler
		void InternalDestroy()
		{
			delete mResource;
			mResource = nullptr;
		}

        int mReferenceCount;
        IResource* mResource;
    };

    typedef std::pair<std::string, ResourceSource> ResourcePair;


public:
    ResourceHandler();
    ~ResourceHandler();

    void Init();
	void ShutDown();

    ResourceHandle GetEmptyResourceHandle();

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

    static ResourceHandle EmptyHandle() { return ResourceHandle("", nullptr); }

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
        if (resource)
        {
            ResourceSource resourceSource(resource);

            mResourceTable[resourceKey] = resourceSource;
            return ResourceHandle(resourceKey, &mResourceTable[resourceKey]);
        }
        else
        {
            return ResourceHandle::EmptyHandle();
        }
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
    if (!resource->Load(resourcePath))
    {
        return nullptr;
    }

    return resource;
}