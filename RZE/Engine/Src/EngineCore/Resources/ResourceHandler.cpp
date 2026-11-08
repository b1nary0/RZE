#include <EngineCore/Resources/ResourceHandler.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>

ResourceHandler::ResourceHandler()
{
}

ResourceHandler::~ResourceHandler()
{
}

void ResourceHandler::Init()
{
	//mAllocator.Init();
}

void ResourceHandler::ShutDown()
{
	U32 liveResourceCount = 0;
	for (auto& resourcePair : mResourceTable)
	{
		ResourceSource& resourceSource = resourcePair.second;
		if (resourceSource.IsValid())
		{
			++liveResourceCount;
			RZE_LOG_DEBUG_ARGS("Resource [%s] still referenced at shutdown.", resourceSource.GetResourcePath().GetRelativePath().c_str());
		}

		// We should always have a resource if it is in the resource table.
		AssertNotNull(resourceSource.GetResource());
		resourceSource.GetResource()->Release();
		resourceSource.InternalDestroy();
	}

	// Just an informative log to notify that we had some referenced resources at the time
	// ShutDown() was called. This is fine.
	RZE_LOG_ARGS("ResourceHandler::ShutDown() encountered %u live resources.", liveResourceCount);

	// Entries are intentionally left in the table: live ResourceHandles still point at them and will
	// decrement their ref counts as they are destroyed. The resources themselves are gone (mResource == nullptr),
	// so those releases are no-ops. The table is freed in ~ResourceHandler, after all handles are gone.
}

bool ResourceHandler::TryBeginAsyncLoad(const std::string& resourceKey, const void* loader)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	AssertNotNull(loader);

	return m_asyncLoads.emplace(resourceKey, loader).second;
}

void ResourceHandler::EndAsyncLoad(const std::string& resourceKey, const void* loader)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	auto iter = m_asyncLoads.find(resourceKey);
	AssertMsg(iter != m_asyncLoads.end() && iter->second == loader, "Ending an async load that this loader didn't begin");
	if (iter != m_asyncLoads.end() && iter->second == loader)
	{
		m_asyncLoads.erase(iter);
	}
}

bool ResourceHandler::IsAsyncLoadInProgress(const std::string& resourceKey) const
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	return m_asyncLoads.find(resourceKey) != m_asyncLoads.end();
}

std::string ResourceHandler::CreateResourceKey(const Filepath& resourcePath)
{
	return resourcePath.GetRelativePath();
}

bool ResourceHandler::HasResource(const Filepath& resourcePath) const
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	return mResourceTable.find(CreateResourceKey(resourcePath)) != mResourceTable.end();
}

ResourceHandle ResourceHandler::GetHandle(const Filepath& resourcePath)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	const std::string resourceKey = CreateResourceKey(resourcePath);

	auto iter = mResourceTable.find(resourceKey);
	if (iter == mResourceTable.end() || iter->second.GetResource() == nullptr)
	{
		return ResourceHandle::EmptyHandle();
	}

	return ResourceHandle(&iter->second, this);
}

ResourceHandle ResourceHandler::RegisterLoadedResource(const Filepath& resourcePath, IResource* resource)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	AssertNotNull(resource);

	const std::string resourceKey = CreateResourceKey(resourcePath);

	auto iter = mResourceTable.find(resourceKey);
	if (iter != mResourceTable.end() && iter->second.GetResource() != nullptr)
	{
		RZE_LOG_WARNING_ARGS("Resource [%s] was already registered; discarding the duplicate.", resourceKey.c_str());
		resource->Release();
		delete resource;

		return ResourceHandle(&iter->second, this);
	}

	ResourceSource resourceSource(resource);
	resourceSource.m_resourcePath = resourcePath;

	// unordered_map never moves its nodes, so the ResourceSource pointer held by handles stays valid
	// as other entries are inserted.
	ResourceSource& storedSource = mResourceTable[resourceKey];
	storedSource = resourceSource;

	return ResourceHandle(&storedSource, this);
}

void ResourceHandler::ReleaseResource(ResourceHandle& resourceHandle)
{
	AssertNotNull(resourceHandle.mResourceSource);

	// Every entry is stored under its own path's key (RegisterLoadedResource, Make), so the entry finds itself
	const Filepath& resourcePath = resourceHandle.mResourceSource->GetResourcePath();
	auto iter = mResourceTable.find(CreateResourceKey(resourcePath));
	AssertMsg(iter != mResourceTable.end() && &iter->second == resourceHandle.mResourceSource, "A handle's entry isn't stored under its own path");
	if (iter != mResourceTable.end() && &iter->second == resourceHandle.mResourceSource)
	{
		ResourceSource& resourceSource = (*iter).second;
		resourceSource.DecreaseRefCount();

		if (!resourceSource.IsReferenced() && resourceSource.GetResource())
		{
			RZE_LOG_DEBUG_ARGS("Releasing resource [%s]", resourcePath.GetRelativePath().c_str());

			// Out of the table and the handle first: releasing it can release the resources it holds (e.g. a mesh's
			// textures), which changes the table
			IResource* const resource = resourceSource.GetResource();
			mResourceTable.erase(iter);

			resourceHandle.mHandler = nullptr;
			resourceHandle.mResourceSource = nullptr;

			resource->Release();
			delete resource;
		}
	}
	else
	{
		RZE_LOG_WARNING_ARGS("Attempting to release resource [%s] but it does not currently exist.", resourcePath.GetRelativePath().c_str());
	}
}

ResourceHandle ResourceHandler::Make(const std::string& resourceID, IResource* resource)
{
	// Keyed by its path like any other entry, so ReleaseResource finds it from that path
	const Filepath resourcePath(resourceID);
	const std::string resourceKey = CreateResourceKey(resourcePath);

	auto iter = mResourceTable.find(resourceKey);
	if (iter == mResourceTable.end())
	{
		RZE_LOG_DEBUG_ARGS("Making resource [%s]", resourceKey.c_str());
		if (resource)
		{
			ResourceSource resourceSource(resource);
			resourceSource.m_resourcePath = resourcePath;

			ResourceSource& storedSource = mResourceTable[resourceKey];
			storedSource = resourceSource;
			return ResourceHandle(&storedSource, this);
		}
		else
		{
			return ResourceHandle::EmptyHandle();
		}
	}

	ResourceSource& resourceSource = (*iter).second;
	return ResourceHandle(&resourceSource, this);
}

const std::unordered_map<std::string, ResourceHandler::ResourceSource>& ResourceHandler::GetResourceTableInternal() const
{
	return mResourceTable;
}

ResourceHandle::ResourceHandle(ResourceHandler::ResourceSource* resourceSource, ResourceHandler* handler)
{
	mHandler = handler;
	mResourceSource = resourceSource;

	// Only the handler makes these, for an entry in its table. Empty handles come from the default constructor.
	AssertNotNull(mResourceSource);
	mResourceSource->IncreaseRefCount();
}

ResourceHandle::ResourceHandle(ResourceHandle&& rhs)
{
	mResourceSource = rhs.mResourceSource;
	rhs.mResourceSource = nullptr;

	mHandler = rhs.mHandler;
	rhs.mHandler = nullptr;
}

ResourceHandle::~ResourceHandle()
{
	ReleaseHeldResource();
}

void ResourceHandle::ReleaseHeldResource()
{
	if (mResourceSource != nullptr && mHandler != nullptr)
	{
		// Decrements the ref count and destroys the resource if this was the last reference
		mHandler->ReleaseResource(*this);
	}

	mResourceSource = nullptr;
	mHandler = nullptr;
}

bool ResourceHandle::IsValid() const
{
	return mResourceSource != nullptr && mResourceSource->IsValid();
}

const Filepath& ResourceHandle::GetResourcePath() const
{
	return mResourceSource->GetResourcePath();
}

const IResource* ResourceHandle::GetResource() const
{
	return mResourceSource != nullptr ? mResourceSource->GetResource() : nullptr;
}

void ResourceHandle::operator=(ResourceHandle&& rhs)
{
	if (this == &rhs)
	{
		return;
	}

	ReleaseHeldResource();

	mResourceSource = rhs.mResourceSource;
	rhs.mResourceSource = nullptr;

	mHandler = rhs.mHandler;
	rhs.mHandler = nullptr;
}

ResourceHandle& ResourceHandle::operator=(const ResourceHandle& rhs)
{
	if (this == &rhs)
	{
		return *this;
	}

	// Take the new reference before dropping the old one, in case both point at the same resource
	if (rhs.mResourceSource != nullptr)
	{
		rhs.mResourceSource->IncreaseRefCount();
	}

	ReleaseHeldResource();

	mResourceSource = rhs.mResourceSource;
	mHandler = rhs.mHandler;

	return *this;
}

int ResourceHandler::ResourceSource::GetRefCount() const
{
	return mReferenceCount;
}
