#pragma once

#include <unordered_map>

#include <EngineCore/Platform/Memory/BlockAllocator.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/FilePath.h>

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

		bool IsReferenced() { return mReferenceCount > 0; }
		bool IsValid() { return IsReferenced() && mResource; }

		void Destroy()
		{
			AssertNotNull(mResource);
			AssertExpr(mReferenceCount <= 0);

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
	ResourceHandle LoadResource(const FilePath& resourcePath, Args... args);

	void ReleaseResource(ResourceHandle& resourceHandle);

	template <class ResourceT>
	ResourceT* GetResource(const ResourceHandle& resourceHandle);

private:
	template <class ResourceT, class... Args>
	IResource* CreateAndLoadResource(const FilePath& resourcePath, Args&&... args);

	std::unordered_map<std::string, ResourceSource> mResourceTable;
};

class ResourceHandle
{
	friend class ResourceHandler;

public:
	ResourceHandle();
	ResourceHandle(const ResourceHandle& rhs);
	ResourceHandle(ResourceHandle&& rhs);
	~ResourceHandle();

	static ResourceHandle EmptyHandle() { return ResourceHandle(); }

	bool IsValid() const;
	const std::string& GetID() const;

	bool operator==(const ResourceHandle& rhs);
	void operator=(const ResourceHandle& rhs);
	void operator=(ResourceHandle&& rhs);

private:
	ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource, ResourceHandler* handler);

	ResourceHandler* mHandler;
	std::string mResourceID;
	ResourceHandler::ResourceSource* mResourceSource;
};

template <class ResourceT, class... Args>
ResourceHandle ResourceHandler::LoadResource(const FilePath& resourcePath, Args... args)
{
	std::string resourceKey = Conversions::CreateResourceKeyFromPath(resourcePath.GetRelativePath());

	auto iter = mResourceTable.find(resourceKey);
	if (iter == mResourceTable.end())
	{
		LOG_CONSOLE_ARGS("Creating resource [%s]", resourceKey.c_str());
		IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath, args...);
		if (resource)
		{
			ResourceSource resourceSource(resource);

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
IResource* ResourceHandler::CreateAndLoadResource(const FilePath& resourcePath, Args&&... args)
{
	IResource* resource = new ResourceT(args...);//new (mAllocator.Allocate(sizeof(ResourceT))) ResourceT;
	AssertNotNull(resource);
	if (!resource->Load(resourcePath))
	{
		return nullptr;
	}

	return resource;
}