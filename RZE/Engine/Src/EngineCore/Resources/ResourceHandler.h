#pragma once

#include <unordered_map>

#include <EngineCore/Platform/Memory/BlockAllocator.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

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
			mReferenceCount = 0;
			mResource = resource;
		}

		void IncreaseRefCount() { ++mReferenceCount; }
		void DecreaseRefCount() { --mReferenceCount; }

		IResource* GetResource() { return mResource; }
		const Filepath& GetResourcePath() const { return m_resourcePath; }

		bool IsReferenced() { return mReferenceCount > 0; }
		bool IsValid() { return IsReferenced() && mResource; }

		void Destroy()
		{
			AssertNotNull(mResource);
			AssertExpr(mReferenceCount <= 0);

			InternalDestroy();
		}

	private:
		// Bypasses asserts needed for regular destruction, 
		// as when we shut down there may be a positive reference count, but thats ok
		// when we just want to shut the system down. We will log each issue in the console
		// for reference when we call Shutdown() on ResourceHandler
		void InternalDestroy()
		{
			delete mResource;
			mResource = nullptr;
		}

		int mReferenceCount;
		Filepath m_resourcePath;
		IResource* mResource = nullptr;
	};

	typedef std::pair<std::string, ResourceSource> ResourcePair;


public:
	ResourceHandler();
	~ResourceHandler();

	void Init();
	void ShutDown();

	ResourceHandle GetEmptyResourceHandle();

	template <class ResourceT, class... Args>
	ResourceHandle LoadResource(const Filepath& resourcePath, Args... args);

	void ReleaseResource(ResourceHandle& resourceHandle);

	template <class ResourceT>
	const ResourceT* GetResource(const ResourceHandle& resourceHandle);

	// @TODO This should be removed when resource instance layer goes in.
	// Do not rely on this function
	ResourceHandle Make(const std::string& resourceID, IResource* resource);

private:
	template <class ResourceT, class... Args>
	IResource* CreateAndLoadResource(const Filepath& resourcePath, Args&&... args);
	
	// #TODO Turn the resource IDs into hashes/guids
	std::unordered_map<std::string, ResourceSource> mResourceTable;
};

// #TODO
// Investigate templating this class so it can be stored as
// ResourceHandle<ResourceType> 
class ResourceHandle
{
	friend class ResourceHandler;

public:
	ResourceHandle();
	ResourceHandle(ResourceHandle&& rhs);
	ResourceHandle(const ResourceHandle& rhs) = delete;
	~ResourceHandle();
	
	static ResourceHandle EmptyHandle() { return ResourceHandle(); }

	bool IsValid() const;
	const std::string& GetID() const;
	const Filepath& GetResourcePath() const;

	bool operator==(const ResourceHandle& rhs);
	ResourceHandle& operator=(const ResourceHandle& rhs);
	void operator=(ResourceHandle&& rhs);

private:
	ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource, ResourceHandler* handler);

	ResourceHandler* mHandler;
	std::string mResourceID; // #TODO Turn this into a hash
	ResourceHandler::ResourceSource* mResourceSource;
};

template <class ResourceT, class... Args>
ResourceHandle ResourceHandler::LoadResource(const Filepath& resourcePath, Args... args)
{
	static_assert(std::is_base_of_v<IResource, ResourceT>);
	std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath.GetRelativePath());

	auto iter = mResourceTable.find(resourceKey);
	if (iter == mResourceTable.end())
	{
		RZE_LOG_ARGS("Creating resource [%s]", resourceKey.c_str());
		IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath, args...);
		if (resource)
		{
			ResourceSource resourceSource(resource);
			resourceSource.m_resourcePath = resourcePath;

			mResourceTable[resourceKey] = resourceSource;
			return ResourceHandle(resourceKey, &mResourceTable[resourceKey], this);
		}
		else
		{
			return ResourceHandle::EmptyHandle();
		}
	}

	ResourceSource& resourceSource = (*iter).second;
	return ResourceHandle(resourceKey, &resourceSource, this);
}

template <class ResourceT>
const ResourceT* ResourceHandler::GetResource(const ResourceHandle& resourceHandle)
{
	// @TODO We dont actually need to do this. We can use our friendship with ResourceHandle to reach into its privates
	auto iter = mResourceTable.find(resourceHandle.GetID());
	if (iter != mResourceTable.end())
	{
		ResourceSource& resourceSource = (*iter).second;
		return static_cast<ResourceT*>(resourceSource.GetResource());
	}

	RZE_LOG_ARGS("Resource with key [%s] does not exist. Request it first.", resourceHandle.GetID().c_str());
	return nullptr;
}

template <class ResourceT, class... Args>
IResource* ResourceHandler::CreateAndLoadResource(const Filepath& resourcePath, Args&&... args)
{
	IResource* resource = new ResourceT(args...);//new (mAllocator.Allocate(sizeof(ResourceT))) ResourceT;
	AssertNotNull(resource);
	if (resource->Load(resourcePath))
	{
		return resource;
	}

	return nullptr;
}