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
		resourceSource.InternalDestroy();
	}

	// Just an informative log to notify that we had some referenced resources at the time
	// ShutDown() was called. This is fine.
	LOG_CONSOLE_ARGS("ResourceHandler::ShutDown() encountered %u live resources.", liveResourceCount);

	mResourceTable.erase(mResourceTable.begin(), mResourceTable.end());
}

ResourceHandle ResourceHandler::GetEmptyResourceHandle()
{
	return ResourceHandle("", nullptr);
}

void ResourceHandler::ReleaseResource(ResourceHandle& resourceHandle)
{
	auto iter = mResourceTable.find(resourceHandle.GetID());
	if (iter != mResourceTable.end())
	{
		LOG_CONSOLE_ARGS("Releasing resource [%s]", resourceHandle.GetID().c_str());

		ResourceSource& resourceSource = (*iter).second;
		resourceSource.DecreaseRefCount();

		if (!resourceSource.IsReferenced())
		{
			resourceSource.Destroy();
			mResourceTable.erase(iter);
		}
	}
	else
	{
		LOG_CONSOLE_ARGS("Attempting to release resource with key [%s] but it does not currently exist.", resourceHandle.GetID().c_str());
	}
}

ResourceHandle::ResourceHandle()
{
	mResourceSource = nullptr;
}

ResourceHandle::ResourceHandle(const std::string& resourceID, ResourceHandler::ResourceSource* resourceSource)
{
	mResourceID = resourceID;
	mResourceSource = resourceSource;
}

ResourceHandle::ResourceHandle(const ResourceHandle& rhs)
{
	mResourceID = rhs.mResourceID;
	mResourceSource = rhs.mResourceSource;

	mResourceSource->IncreaseRefCount();
}

ResourceHandle::~ResourceHandle()
{
	if (mResourceSource)
	{
		mResourceSource->DecreaseRefCount();
	}
}

bool ResourceHandle::IsValid() const
{
	return !mResourceID.empty() && mResourceSource->IsValid();
}

const std::string& ResourceHandle::GetID() const
{
	return mResourceID;
}

void ResourceHandle::operator=(const ResourceHandle& rhs)
{
	mResourceID = rhs.mResourceID;
	mResourceSource = rhs.mResourceSource;

	mResourceSource->IncreaseRefCount();
}

bool ResourceHandle::operator==(const ResourceHandle& rhs)
{
	return rhs.mResourceID == mResourceID;
}
