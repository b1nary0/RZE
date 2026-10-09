#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <EngineCore/Resources/ResourceHandler.h>
#include <EngineCore/Threading/JobSystem/Job.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

class AsyncFrameBudget;
class AsyncOperation;

// Loads a set of resources (and whatever they depend on) without stalling the main thread:
//  1. IResource::LoadCPU runs on a worker thread for each resource.
//  2. IResource::RequestDependencies lets a resource pull in others (e.g. a mesh requests its textures).
//  3. Once its dependencies are settled, IResource::FinalizeStep runs on the main thread, time-sliced by the
//     frame budget, and the resource is registered with the ResourceHandler.
//
// Resources that are already loaded are simply referenced, and resources that don't support async loading
// fall back to a synchronous ResourceHandler::LoadResource. Every resolved resource is held by the batch until
// ReleaseHeldResources() so it can't be unloaded before whoever requested it picks it up.
//
// Main thread only. Worker jobs are pushed through the owning AsyncOperation, which guarantees they complete
// before the operation (and therefore this batch) is torn down.
class AsyncResourceBatch
{
public:
	explicit AsyncResourceBatch(AsyncOperation& owner);
	~AsyncResourceBatch();

	AsyncResourceBatch(const AsyncResourceBatch&) = delete;
	AsyncResourceBatch& operator=(const AsyncResourceBatch&) = delete;

public:
	// Requests a resource. Duplicate requests (by resource key) are ignored. When called from within
	// IResource::RequestDependencies, the requested resource becomes a dependency of the calling resource.
	template <class TResource, class... Args>
	void Request(const Filepath& resourcePath, Args... args);

	void Tick(AsyncFrameBudget& budget);

	// True once every requested resource has either been resolved or has failed.
	bool IsComplete() const;

	U32 GetRequestedCount() const { return static_cast<U32>(m_entries.size()); }
	U32 GetCPULoadedCount() const { return m_cpuLoadedCount; }
	U32 GetSettledCount() const { return m_settledCount; }
	U32 GetFailedCount() const { return m_failedCount; }

	const std::string& GetLastProcessedPath() const { return m_lastProcessedPath; }

	// Drops the batch's references to resolved resources.
	void ReleaseHeldResources();

	// Destroys any resources that were loaded but never registered. Worker jobs must have finished.
	void DiscardPending();

private:
	enum class EEntryState : U8
	{
		LoadingCPU,
		// CPU load finished; waiting for dependencies and/or main-thread finalization.
		Finalizing,
		Resolved,
		Failed
	};

	struct Entry
	{
		Filepath Path;
		std::string Key;
		EEntryState State = EEntryState::LoadingCPU;

		// Owned until registered with the ResourceHandler.
		std::unique_ptr<IResource> Resource;
		Threading::JobHandle CPUJob;
		// Written by the CPU job, read on the main thread only after CPUJob reports completion.
		bool CPULoadSucceeded = false;

		std::vector<Entry*> Dependencies;
	};

private:
	static ResourceHandler& GetResourceHandler();

	// Returns the new entry if the resource needs loading, or nullptr if it was already requested or already loaded.
	Entry* BeginRequest(const Filepath& resourcePath);
	void StartCPULoad(Entry& entry, std::unique_ptr<IResource> resource);
	void ResolveWithHandle(Entry& entry, ResourceHandle handle);

	void PollCPULoads();
	void FinalizeReadyEntries(AsyncFrameBudget& budget);

	bool AreDependenciesSettled(const Entry& entry) const;
	void Register(Entry& entry);
	void FailEntry(Entry& entry, const char* reason);
	void MarkCPULoaded(Entry& entry);

private:
	AsyncOperation& m_owner;

	// unique_ptr so entries keep a stable address for worker jobs and dependency links.
	std::vector<std::unique_ptr<Entry>> m_entries;
	std::unordered_map<std::string, Entry*> m_entriesByKey;

	std::vector<ResourceHandle> m_heldResources;

	// Set while a resource's RequestDependencies is running.
	Entry* m_currentParent = nullptr;

	U32 m_cpuLoadedCount = 0;
	U32 m_settledCount = 0;
	U32 m_failedCount = 0;
	std::string m_lastProcessedPath;
};

template <class TResource, class... Args>
void AsyncResourceBatch::Request(const Filepath& resourcePath, Args... args)
{
	static_assert(std::is_base_of_v<IResource, TResource>);

	Entry* const entry = BeginRequest(resourcePath);
	if (entry == nullptr)
	{
		return;
	}

	std::unique_ptr<IResource> resource = std::make_unique<TResource>(args...);
	if (resource->SupportsAsyncLoad())
	{
		StartCPULoad(*entry, std::move(resource));
	}
	else
	{
		resource.reset();
		ResolveWithHandle(*entry, GetResourceHandler().LoadResource<TResource>(resourcePath, args...));
	}
}
