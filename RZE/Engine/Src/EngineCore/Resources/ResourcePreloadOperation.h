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

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;
	void OnCleanup() override;

private:
	ResourceDependencyList m_resources;
	// Holds what it loaded until the operation is destroyed
	AsyncResourceBatch m_batch;
};
