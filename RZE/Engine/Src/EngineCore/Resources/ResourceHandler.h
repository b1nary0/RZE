#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

// Before ResourceHandler uses it: a friend declaration alone doesn't declare the name for the rest of the class
class ResourceHandle;

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
		// Empty for an unnamed resource
		const Filepath& GetResourcePath() const { return m_resourcePath; }
		bool IsUnnamed() const { return m_isUnnamed; }
		// What tools and logs show: the relative path, or an unnamed resource's display name
		const std::string& GetDisplayName() const { return m_isUnnamed ? m_displayName : m_resourcePath.GetRelativePath(); }

		int GetRefCount() const;

		bool IsReferenced() { return mReferenceCount > 0; }
		bool IsValid() { return IsReferenced() && mResource; }

	private:
		// When we shut down there may be a positive reference count, but thats ok
		// when we just want to shut the system down. We will log each issue in the console
		// for reference when we call Shutdown() on ResourceHandler
		void InternalDestroy()
		{
			delete mResource;
			mResource = nullptr;
		}

		int mReferenceCount;
		Filepath m_resourcePath;
		// Unnamed resources (AddUnnamedResource) have no path, only a name for people to read
		bool m_isUnnamed = false;
		std::string m_displayName;
		IResource* mResource = nullptr;
	};


public:
	ResourceHandler();
	~ResourceHandler();

	void Init();
	void ShutDown();

	// NOTE: ResourceHandler and ResourceHandle are main-thread only (the table and ref counts are unsynchronized).
	// Use AsyncResourceBatch to load resources off the main thread.

	// The key a resource path is stored under: its relative path, extension and all, with this platform's separators
	// (Filepath turns either into them). Matching is exact, so name files as they are on disk.
	static std::string CreateResourceKey(const Filepath& resourcePath);

	bool HasResource(const Filepath& resourcePath) const;

	// Handle to an already-loaded resource, or an empty handle if it isn't loaded.
	ResourceHandle GetHandle(const Filepath& resourcePath);

	// Adds a resource that was loaded outside of LoadResource (e.g. asynchronously) and takes ownership of it.
	// If a resource is already registered for the path, the incoming one is released and destroyed and a handle
	// to the existing one is returned.
	ResourceHandle RegisterLoadedResource(const Filepath& resourcePath, IResource* resource);

	template <class ResourceT, class... Args>
	ResourceHandle LoadResource(const Filepath& resourcePath, Args... args);

	// Drops the handle's reference, and releases and deletes the resource straight away if it was the last one. What
	// the render thread may still use (GPU buffers, uploaded texels, textures the UI draws) lives on in its commands.
	void ReleaseResource(ResourceHandle& resourceHandle);

	// Async loads in progress, so a second request for a resource waits for the first instead of loading and uploading
	// it again. loader identifies who's loading it (e.g. an AsyncResourceBatch); only it can end the load. Returns false
	// if someone else is already loading it.
	bool TryBeginAsyncLoad(const std::string& resourceKey, const void* loader);
	// Called once loader has registered the resource, or given up on it
	void EndAsyncLoad(const std::string& resourceKey, const void* loader);
	bool IsAsyncLoadInProgress(const std::string& resourceKey) const;

	// Takes ownership of a resource made in code rather than loaded from a file (e.g. GifChatComponent's frames), so it's
	// counted, released with its last handle and destroyed at ShutDown like any other. Nothing can look it up:
	// HasResource and GetHandle never find it, and each call adds a separate resource whatever its display name, which
	// is only for tools and logs. Empty handle for a null resource.
	ResourceHandle AddUnnamedResource(std::unique_ptr<IResource>&& resource, const std::string& displayName);

	// The resource the handle holds, as ResourceT; null for an empty handle
	template <class ResourceT>
	const ResourceT* GetResource(const ResourceHandle& resourceHandle);

	// For tools: how many resources are held, and each one's display name (GetDisplayName), reference count and
	// whether it's unnamed, in no particular order
	size_t GetResourceCount() const;
	void ForEachResource(const std::function<void(const std::string& displayName, int referenceCount, bool isUnnamed)>& visit) const;

private:
	template <class ResourceT, class... Args>
	IResource* CreateAndLoadResource(const Filepath& resourcePath, Args&&... args);

	// Takes the entry, path or unnamed, out of its container, which destroys it
	void EraseEntry(const ResourceSource& resourceSource);

	// Key (CreateResourceKey of the entry's path) -> entry. Keys never leave ResourceHandler and AsyncResourceBatch,
	// and an entry carries its own path for display, so the key's type can change there alone if lookups ever show up
	// in a profile. Handles point at their entry, so they need no key.
	std::unordered_map<std::string, ResourceSource> mResourceTable;
	// Entries for unnamed resources, by address: nothing looks them up by name. unique_ptr, so they stay put as
	// handles point at them.
	std::unordered_map<const ResourceSource*, std::unique_ptr<ResourceSource>> m_unnamedResources;

	// Resource key -> who's loading it
	std::unordered_map<std::string, const void*> m_asyncLoads;
};

// A counted reference to a resource in ResourceHandler. TypedResourceHandle (TypedResourceHandle.h) holds one by its
// resource's type, with the resource resolved.
class ResourceHandle
{
	friend class ResourceHandler;

public:
	ResourceHandle() = default;
	ResourceHandle(ResourceHandle&& rhs);
	ResourceHandle(const ResourceHandle& rhs) = delete;
	~ResourceHandle();

	static ResourceHandle EmptyHandle() { return ResourceHandle(); }

	bool IsValid() const;
	// Empty for an unnamed resource (ResourceHandler::AddUnnamedResource)
	const Filepath& GetResourcePath() const;
	// The resource this holds, or null when empty
	const IResource* GetResource() const;

	ResourceHandle& operator=(const ResourceHandle& rhs);
	void operator=(ResourceHandle&& rhs);

private:
	ResourceHandle(ResourceHandler::ResourceSource* resourceSource, ResourceHandler* handler);

	// Drops this handle's reference and resets it to empty.
	void ReleaseHeldResource();

	ResourceHandler* mHandler = nullptr;
	// Its entry in the handler's table, which stays put while handles point at it
	ResourceHandler::ResourceSource* mResourceSource = nullptr;
};

template <class ResourceT, class... Args>
ResourceHandle ResourceHandler::LoadResource(const Filepath& resourcePath, Args... args)
{
	static_assert(std::is_base_of_v<IResource, ResourceT>);

	ResourceHandle existingHandle = GetHandle(resourcePath);
	if (existingHandle.IsValid())
	{
		return existingHandle;
	}

	RZE_LOG_DEBUG_ARGS("Creating resource [%s]", resourcePath.GetRelativePath().c_str());
	IResource* resource = CreateAndLoadResource<ResourceT>(resourcePath, args...);
	if (resource == nullptr)
	{
		return ResourceHandle::EmptyHandle();
	}

	return RegisterLoadedResource(resourcePath, resource);
}

template <class ResourceT>
const ResourceT* ResourceHandler::GetResource(const ResourceHandle& resourceHandle)
{
	// The handle points at its entry in the table, so there's no key to look up
	return static_cast<const ResourceT*>(resourceHandle.GetResource());
}

template <class ResourceT, class... Args>
IResource* ResourceHandler::CreateAndLoadResource(const Filepath& resourcePath, Args&&... args)
{
	IResource* resource = new ResourceT(args...);
	AssertNotNull(resource);
	if (resource->Load(resourcePath))
	{
		return resource;
	}

	resource->Release();
	delete resource;
	return nullptr;
}