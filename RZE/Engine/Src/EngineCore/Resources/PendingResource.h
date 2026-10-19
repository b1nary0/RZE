#pragma once

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Resources/ResourceDependencyList.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Platform/Filepath.h>

#include <memory>

class ResourcePreloadOperation;

// One resource loaded in the background, e.g. the image a tile shows or a mesh about to be swapped in. Load takes it
// straight away if something already has it loaded, or else starts loading it (ResourcePreloadOperation). IsLoading,
// polled each frame, picks up the handle once it's in, and the handle then keeps the resource loaded for as long as this
// holds it. Reset, loading something else, or destroying it cancels a load still running.
//
// Move-only. Main thread only.
class PendingResource
{
public:
	PendingResource() = default;
	// Cancels a load still running
	~PendingResource();

	PendingResource(PendingResource&& other) = default;
	// Cancels this one's load still running, then takes other's
	PendingResource& operator=(PendingResource&& other);
	PendingResource(const PendingResource&) = delete;
	PendingResource& operator=(const PendingResource&) = delete;

public:
	// Loads the resource at path in place of whatever this held or was loading. flags are EAsyncOperationFlags, and the
	// load is called "Loading <file name>". Returns the load it started, e.g. to add to an AsyncOperationGroup, or null
	// if it took the resource straight away.
	template <class TResource>
	std::shared_ptr<AsyncOperation> Load(const Filepath& path, U32 flags = EAsyncOperationFlags::Background);

	// Whether its load is still running. Picks up the handle once the resource is in, so call it each frame while waiting.
	bool IsLoading();
	// Empty while it's loading, and once it has failed
	const ResourceHandle& GetHandle() const { return m_handle; }
	// Whether its load finished without the resource, e.g. a missing file
	bool HasFailed() const { return m_hasFailed; }
	const Filepath& GetPath() const { return m_path; }

	// Cancels a load still running and lets go of the resource
	void Reset();

private:
	// Takes the resource if it's loaded, or else starts loading resources
	std::shared_ptr<AsyncOperation> StartLoad(const Filepath& path, ResourceDependencyList&& resources, U32 flags);

private:
	Filepath m_path;
	ResourceHandle m_handle;
	// Until it's finished
	std::shared_ptr<ResourcePreloadOperation> m_load;
	bool m_hasFailed = false;
};

template <class TResource>
std::shared_ptr<AsyncOperation> PendingResource::Load(const Filepath& path, U32 flags)
{
	ResourceDependencyList resources;
	resources.Add<TResource>(path.GetRelativePath());
	return StartLoad(path, std::move(resources), flags);
}
