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
	const auto destroy = [&liveResourceCount](ResourceSource& resourceSource)
	{
		if (resourceSource.IsValid())
		{
			++liveResourceCount;
			RZE_LOG_DEBUG_ARGS("Resource [%s] still referenced at shutdown.", resourceSource.GetDisplayName().c_str());
		}

		// We should always have a resource if it is in the resource table.
		AssertNotNull(resourceSource.GetResource());
		resourceSource.GetResource()->Release();
		resourceSource.InternalDestroy();
	};
	for (auto& resourcePair : mResourceTable)
	{
		destroy(resourcePair.second);
	}
	for (auto& unnamedPair : m_unnamedResources)
	{
		destroy(*unnamedPair.second);
	}

	// Just an informative log to notify that we had some referenced resources at the time
	// ShutDown() was called. This is fine.
	RZE_LOG_ARGS("ResourceHandler::ShutDown() encountered %u live resources.", liveResourceCount);

	// Entries, path and unnamed, are intentionally left in place: live ResourceHandles still point at them and will
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
	ResourceSource* const resourceSource = resourceHandle.mResourceSource;
	AssertNotNull(resourceSource);

	resourceSource->DecreaseRefCount();
	// After ShutDown the entry stays but its resource is gone, so there's nothing left to release
	if (resourceSource->IsReferenced() || resourceSource->GetResource() == nullptr)
	{
		return;
	}

	RZE_LOG_DEBUG_ARGS("Releasing resource [%s]", resourceSource->GetDisplayName().c_str());

	// Out of the table and the handle first: releasing it can release the resources it holds (e.g. a mesh's textures),
	// which changes the table
	IResource* const resource = resourceSource->GetResource();
	resourceHandle.mHandler = nullptr;
	resourceHandle.mResourceSource = nullptr;
	EraseEntry(*resourceSource);

	resource->Release();
	delete resource;
}

void ResourceHandler::EraseEntry(const ResourceSource& resourceSource)
{
	if (resourceSource.IsUnnamed())
	{
		if (m_unnamedResources.erase(&resourceSource) == 0)
		{
			AssertMsg(false, "An unnamed resource's entry isn't in the handler");
		}
		return;
	}

	// Every path entry is stored under its own path's key (RegisterLoadedResource), so the entry finds itself
	auto iter = mResourceTable.find(CreateResourceKey(resourceSource.GetResourcePath()));
	if (iter == mResourceTable.end() || &iter->second != &resourceSource)
	{
		AssertMsg(false, "A resource's entry isn't stored under its own path");
		RZE_LOG_WARNING_ARGS("Resource [%s] isn't stored under its own path; leaving its entry.", resourceSource.GetDisplayName().c_str());
		return;
	}
	mResourceTable.erase(iter);
}

ResourceHandle ResourceHandler::AddUnnamedResource(std::unique_ptr<IResource>&& resource, const std::string& displayName)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	if (resource == nullptr)
	{
		return ResourceHandle::EmptyHandle();
	}

	std::unique_ptr<ResourceSource> resourceSource = std::make_unique<ResourceSource>(resource.release());
	resourceSource->m_isUnnamed = true;
	resourceSource->m_displayName = displayName;

	ResourceSource* const storedSource = resourceSource.get();
	m_unnamedResources.emplace(storedSource, std::move(resourceSource));

	RZE_LOG_DEBUG_ARGS("Adding unnamed resource [%s]", displayName.c_str());
	return ResourceHandle(storedSource, this);
}

size_t ResourceHandler::GetResourceCount() const
{
	return mResourceTable.size() + m_unnamedResources.size();
}

void ResourceHandler::ForEachResource(const std::function<void(const std::string& displayName, int referenceCount, bool isUnnamed)>& visit) const
{
	for (const auto& resourcePair : mResourceTable)
	{
		visit(resourcePair.second.GetDisplayName(), resourcePair.second.GetRefCount(), false);
	}
	for (const auto& unnamedPair : m_unnamedResources)
	{
		visit(unnamedPair.second->GetDisplayName(), unnamedPair.second->GetRefCount(), true);
	}
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
