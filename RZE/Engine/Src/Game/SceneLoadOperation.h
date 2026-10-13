#pragma once

#include <memory>
#include <string>

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/WeightedProgress.h>
#include <EngineCore/Resources/AsyncResourceBatch.h>
#include <EngineCore/Resources/ResourceDependencyList.h>

#include <Utils/Platform/Filepath.h>

#include <RapidJSON/fwd.h>

class GameObjectLoader;
class GameScene;

// Loads a scene asynchronously into the given scene, active or not, replacing its contents:
//  1. Parsing          (worker)  read + parse the scene file and gather the resources its components need.
//  2. LoadingResources (workers + main, time-sliced) preload those resources via AsyncResourceBatch.
//  3. Instantiating    (main, time-sliced) create the game objects in file order; resource loads are now cache hits.
//
// On failure the partially loaded scene is unloaded, leaving it empty.
class SceneLoadOperation final : public AsyncOperation
{
public:
	// An invalid scenePath loads the default (new) scene. flags are EAsyncOperationFlags.
	SceneLoadOperation(GameScene& scene, const Filepath& scenePath, U32 flags = EAsyncOperationFlags::Blocking);
	// For a scene the engine doesn't own: the operation keeps it alive until it's released, so its owner can let go of
	// it while it's still loading (e.g. HeadlessScene cancelling one load for another)
	SceneLoadOperation(std::shared_ptr<GameScene> scene, const Filepath& scenePath, U32 flags);
	~SceneLoadOperation() override;

public:
	const Filepath& GetScenePath() const { return m_scenePath; }

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;
	void OnFinished(EAsyncOperationState finalState) override;
	void OnCleanup() override;

private:
	enum class EPhase : U8
	{
		Parsing,
		LoadingResources,
		Instantiating
	};

	void TickParsing();
	void TickLoadingResources(AsyncFrameBudget& budget);
	void TickInstantiating(AsyncFrameBudget& budget);

	void UpdateProgressAndStatus();

private:
	// Null for a scene someone else keeps alive
	std::shared_ptr<GameScene> m_sceneOwner;
	GameScene& m_scene;
	const Filepath m_scenePath;

	EPhase m_phase = EPhase::Parsing;

	// Written by the parse job; only read on the main thread once m_parseJob reports completion.
	Threading::JobHandle m_parseJob;
	std::unique_ptr<rapidjson::Document> m_document;
	ResourceDependencyList m_dependencies;
	U32 m_objectCount = 0;
	bool m_parseSucceeded = false;
	std::string m_parseError;

	AsyncResourceBatch m_resourceBatch;
	U32 m_nextObjectIndex = 0;
	// Links parents across the batch while instantiating
	std::unique_ptr<GameObjectLoader> m_loader;

	WeightedProgress m_progress;
	U32 m_parseStage = 0;
	U32 m_cpuLoadStage = 0;
	U32 m_finalizeStage = 0;
	U32 m_instantiateStage = 0;
};
