#pragma once

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/WeightedProgress.h>
#include <EngineCore/Resources/AsyncResourceBatch.h>
#include <EngineCore/Resources/ResourceDependencyList.h>

// Loads a list of resources, and whatever they depend on, in the background through an AsyncResourceBatch, and keeps
// them loaded for as long as the operation is held. Whoever started it can then load them synchronously (e.g.
// RenderComponent::LoadMesh) without stalling the main thread. A resource that fails to load is logged and left out;
// the operation still succeeds.
//
// Its progress counts the files read and then the resources uploaded, a part at a time for those that upload in several
// steps (IResource::GetFinalizeProgress). Its status says which of those it's doing and the file it's waiting on.
class ResourcePreloadOperation final : public AsyncOperation
{
public:
	// flags are EAsyncOperationFlags. name is what progress displays call it, e.g. "Loading lion.png".
	ResourcePreloadOperation(ResourceDependencyList resources, U32 flags, const std::string& name = "Preloading resources");

	// The handle to a resource it was asked for (or one that pulled in), once that has loaded, even while it's still
	// loading others; e.g. to swap a mesh in when its preload finishes (see RenderComponent::SetMeshInstance). Empty
	// for one that failed, hasn't loaded yet, or it wasn't asked for. Loads nothing. The handle is a reference of its
	// own, so it keeps the resource loaded after the operation is gone. Main thread only.
	ResourceHandle GetHandle(const Filepath& resourcePath) const;

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;
	void OnCleanup() override;

private:
	void UpdateProgressAndStatus();

private:
	ResourceDependencyList m_resources;
	// Holds what it loaded until the operation is destroyed
	AsyncResourceBatch m_batch;

	WeightedProgress m_progress;
	U32 m_readStage = 0;
	U32 m_uploadStage = 0;
};
