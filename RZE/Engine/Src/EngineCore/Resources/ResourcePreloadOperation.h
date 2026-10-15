#pragma once

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Resources/AsyncResourceBatch.h>
#include <EngineCore/Resources/ResourceDependencyList.h>

// Loads a list of resources, and whatever they depend on, in the background through an AsyncResourceBatch, and keeps
// them loaded for as long as the operation is held. Whoever started it can then load them synchronously (e.g.
// RenderComponent::LoadMesh) without stalling the main thread. A resource that fails to load is logged and left out;
// the operation still succeeds.
class ResourcePreloadOperation final : public AsyncOperation
{
public:
	// flags are EAsyncOperationFlags
	ResourcePreloadOperation(ResourceDependencyList resources, U32 flags);

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
	ResourceDependencyList m_resources;
	// Holds what it loaded until the operation is destroyed
	AsyncResourceBatch m_batch;
};
