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
		}

		// We should always have a resource if it is in the resource table.
		AssertNotNull(resourceSource.GetResource());
		resourceSource.GetResource()->Release();
		resourceSource.InternalDestroy();
	}

	// Just an informative log to notify that we had some referenced resources at the time
	// ShutDown() was called. This is fine.
	LOG_CONSOLE_ARGS("ResourceHandler::ShutDown() encountered %u live resources.", liveResourceCount);

	mResourceTable.erase(mResourceTable.begin(), mResourceTable.end());
}

ResourceHandle ResourceHandler::GetEmptyResourceHandle()
{
	return ResourceHandle("", nullptr, this);
}

void ResourceHandler::ReleaseResource(ResourceHandle& resourceHandle)
{
	auto iter = mResourceTable.find(resourceHandle.GetID());
	if (iter != mResourceTable.end())
	{
		ResourceSource& resourceSource = (*iter).second;
		resourceSource.DecreaseRefCount();

		if (!resourceSource.IsReferenced() && resourceSource.GetResource())
		{
			LOG_CONSOLE_ARGS("Releasing resource [%s]", resourceHandle.GetID().c_str());

			resourceSource.GetResource()->Release();
			resourceSource.Destroy();
			mResourceTable.erase(iter);

			resourceHandle = ResourceHandle::EmptyHandle();
		}
	}
	else
	{
		LOG_CONSOLE_ARGS("Attempting to release resource with key [%s] but it does not currently exist.", resourceHandle.GetID().c_str());
	}
}

ResourceHandle::ResourceHandle()
	: mResourceID("NO_RESOURCE")
	, mResourceSource(nullptr)
	, mHandler(nullptr)
{
}

ResourceHandle::ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource, ResourceHandler* handler)
{
	mHandler = handler;
	mResourceID = resourceID;
	mResourceSource = resourceSource;

	// #TODO(Josh::Quick fix for ::EmptyHandle but really this shouldn't happen. Revisit.)
	if (resourceSource != nullptr)
	{
		mResourceSource->IncreaseRefCount();
	}
}

ResourceHandle::ResourceHandle(const ResourceHandle& rhs)
{
	mResourceID = rhs.mResourceID;
	mResourceSource = rhs.mResourceSource;
	mHandler = rhs.mHandler;

	if (mResourceSource != nullptr)
	{
		mResourceSource->IncreaseRefCount();
	}
}


ResourceHandle::ResourceHandle(ResourceHandle&& rhs)
{
	mResourceID = rhs.mResourceID;
	rhs.mResourceID = "";

	mResourceSource = rhs.mResourceSource;
	rhs.mResourceSource = nullptr;

	mHandler = rhs.mHandler;
	rhs.mHandler = nullptr;
}

ResourceHandle::~ResourceHandle()
{
	if (mResourceSource != nullptr)
	{
		mResourceSource->DecreaseRefCount();
		if (!mResourceSource->IsReferenced() && mResourceSource->GetResource() != nullptr)
		{
			mHandler->ReleaseResource(*this);
		}
	}
}

bool ResourceHandle::IsValid() const
{
	return !mResourceID.empty() 
		&& (mResourceSource != nullptr && mResourceSource->IsValid());
}

const std::string& ResourceHandle::GetID() const
{
	return mResourceID;
}

const FilePath& ResourceHandle::GetResourcePath() const
{
	return mResourceSource->GetResourcePath();
}

void ResourceHandle::operator=(ResourceHandle&& rhs)
{
	mResourceID = rhs.mResourceID;
	rhs.mResourceID = "";

	mResourceSource = rhs.mResourceSource;
	rhs.mResourceSource = nullptr;

	mHandler = rhs.mHandler;
	rhs.mHandler = nullptr;
}

void ResourceHandle::operator=(const ResourceHandle& rhs)
{
	mResourceID = rhs.mResourceID;
	mResourceSource = rhs.mResourceSource;
	mHandler = rhs.mHandler;

	mResourceSource->IncreaseRefCount();
}

bool ResourceHandle::operator==(const ResourceHandle& rhs)
{
	return rhs.mResourceID == mResourceID;
}
