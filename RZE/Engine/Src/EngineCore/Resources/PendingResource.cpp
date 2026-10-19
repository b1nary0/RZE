#include <StdAfx.h>
#include <EngineCore/Resources/PendingResource.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>

PendingResource::~PendingResource()
{
	Reset();
}

PendingResource& PendingResource::operator=(PendingResource&& other)
{
	if (this != &other)
	{
		Reset();
		m_path = std::move(other.m_path);
		m_handle = std::move(other.m_handle);
		m_load = std::move(other.m_load);
		m_hasFailed = other.m_hasFailed;
	}
	return *this;
}

bool PendingResource::IsLoading()
{
	if (m_load == nullptr)
	{
		return false;
	}

	m_handle = m_load->GetHandle(m_path);
	if (!m_handle.IsValid() && !m_load->IsFinished())
	{
		return true;
	}

	// The handle keeps it loaded from here on; a load that finished without it has logged why
	m_hasFailed = !m_handle.IsValid();
	m_load = nullptr;
	return false;
}

void PendingResource::Reset()
{
	if (m_load != nullptr && !m_load->IsFinished())
	{
		m_load->RequestCancel();
	}
	m_load = nullptr;
	m_handle = ResourceHandle::EmptyHandle();
	m_path = Filepath();
	m_hasFailed = false;
}

std::shared_ptr<AsyncOperation> PendingResource::StartLoad(const Filepath& path, ResourceDependencyList&& resources, U32 flags)
{
	Reset();
	m_path = path;

	m_handle = RZE().GetResourceHandler().GetHandle(path);
	if (m_handle.IsValid())
	{
		return nullptr;
	}

	m_load = std::make_shared<ResourcePreloadOperation>(std::move(resources), flags, "Loading " + path.GetFileName());
	RZE().GetAsyncOperationManager().Start(m_load);
	return m_load;
}
