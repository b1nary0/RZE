#include <StdAfx.h>
#include <Game/SceneLoadOperation.h>

#include <EngineCore/Async/AsyncFrameBudget.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObjectComponent.h>

#include <RapidJSON/document.h>

namespace
{
	constexpr float k_parseWeight = 0.05f;
	constexpr float k_cpuLoadWeight = 0.45f;
	constexpr float k_finalizeWeight = 0.35f;
	constexpr float k_instantiateWeight = 0.15f;

	// Runs on a worker thread: only reads the document and the (immutable after startup) component registry.
	void GatherSceneDependencies(rapidjson::Document& document, ResourceDependencyList& outDependencies, U32& outObjectCount)
	{
		outObjectCount = 0;

		rapidjson::Value::MemberIterator root = document.FindMember("gameobjects");
		if (root == document.MemberEnd())
		{
			return;
		}

		const GameObjectComponentRegistry::ComponentNameIDMap& componentInfo = GameObjectComponentRegistry::GetAllComponentReflectData();

		for (auto object = root->value.MemberBegin(); object != root->value.MemberEnd(); ++object)
		{
			++outObjectCount;

			if (!object->value.IsObject())
			{
				continue;
			}

			rapidjson::Value::MemberIterator components = object->value.FindMember("components");
			if (components == object->value.MemberEnd() || !components->value.IsObject())
			{
				continue;
			}

			for (const auto& componentPair : componentInfo)
			{
				GameObjectComponentRegistry::ResourceDependencyGatherer gatherer = GameObjectComponentRegistry::GetResourceDependencyGatherer(componentPair.first);
				if (gatherer == nullptr)
				{
					continue;
				}

				rapidjson::Value::MemberIterator componentData = components->value.FindMember(componentPair.second.c_str());
				if (componentData != components->value.MemberEnd())
				{
					gatherer(componentData->value, outDependencies);
				}
			}
		}
	}
}

SceneLoadOperation::SceneLoadOperation(GameScene& scene, const Filepath& scenePath)
	: AsyncOperation("Loading Scene", EAsyncOperationFlags::Blocking)
	, m_scene(scene)
	, m_scenePath(scenePath.IsValid() ? scenePath : GameScene::GetDefaultScenePath())
	, m_resourceBatch(*this)
{
	m_parseStage = m_progress.AddStage(k_parseWeight);
	m_cpuLoadStage = m_progress.AddStage(k_cpuLoadWeight);
	m_finalizeStage = m_progress.AddStage(k_finalizeWeight);
	m_instantiateStage = m_progress.AddStage(k_instantiateWeight);
}

SceneLoadOperation::~SceneLoadOperation()
{
}

void SceneLoadOperation::OnStart()
{
	OPTICK_EVENT();
	AssertMsg(&m_scene == &RZE().GetActiveScene(), "GameObject::Load resolves parents through the active scene, so only the active scene can be loaded into.");

	m_scene.Unload();
	m_scene.SetCurrentScenePath(m_scenePath);

	SetStatusText("Reading " + m_scenePath.GetRelativePath() + "...");

	m_parseJob = PushWorkerJob(Threading::Job::Task([this]()
		{
			OPTICK_EVENT("SceneLoadOperation parse");

			if (IsCancellationRequested())
			{
				return;
			}

			std::unique_ptr<rapidjson::Document> document = std::make_unique<rapidjson::Document>();
			if (!GameScene::ParseSceneFile(m_scenePath, *document, m_parseError))
			{
				return;
			}

			GatherSceneDependencies(*document, m_dependencies, m_objectCount);

			m_document = std::move(document);
			m_parseSucceeded = true;
		}));
}

void SceneLoadOperation::OnTick(AsyncFrameBudget& budget)
{
	OPTICK_EVENT();

	// Phases fall through within a frame so no frame is wasted between them.
	if (m_phase == EPhase::Parsing)
	{
		TickParsing();
	}

	if (m_phase == EPhase::LoadingResources)
	{
		TickLoadingResources(budget);
	}

	if (m_phase == EPhase::Instantiating)
	{
		TickInstantiating(budget);
	}

	UpdateProgressAndStatus();
}

void SceneLoadOperation::TickParsing()
{
	if (!m_parseJob.IsComplete())
	{
		return;
	}

	if (m_parseJob.WasAbandoned() || !m_parseSucceeded)
	{
		Fail(m_parseError.empty() ? "Failed to read scene [" + m_scenePath.GetRelativePath() + "]." : m_parseError);
		return;
	}

	m_progress.MarkStageComplete(m_parseStage);

	m_dependencies.RequestAll(m_resourceBatch);
	m_phase = EPhase::LoadingResources;
}

void SceneLoadOperation::TickLoadingResources(AsyncFrameBudget& budget)
{
	m_resourceBatch.Tick(budget);

	if (m_resourceBatch.IsComplete())
	{
		if (m_resourceBatch.GetFailedCount() > 0)
		{
			// Not fatal: affected objects are created without those resources, same as a synchronous load.
			RZE_LOG_ARGS("Scene [%s]: %u of %u resources failed to load.", m_scenePath.GetRelativePath().c_str(),
				m_resourceBatch.GetFailedCount(), m_resourceBatch.GetRequestedCount());
		}

		m_progress.MarkStageComplete(m_cpuLoadStage);
		m_progress.MarkStageComplete(m_finalizeStage);
		m_phase = EPhase::Instantiating;
	}
}

void SceneLoadOperation::TickInstantiating(AsyncFrameBudget& budget)
{
	rapidjson::Value::MemberIterator root = m_document->FindMember("gameobjects");
	if (root == m_document->MemberEnd())
	{
		Complete();
		return;
	}

	rapidjson::Value& objects = root->value;
	const U32 objectCount = objects.MemberCount();

	// File order matters: a child's "parent" must already be in the scene when it is loaded.
	while (m_nextObjectIndex < objectCount && budget.TryConsume(ResourceFinalizeCost()))
	{
		rapidjson::Value::MemberIterator object = objects.MemberBegin() + m_nextObjectIndex;
		m_scene.DeserializeGameObject(object->name.GetString(), object->value);

		++m_nextObjectIndex;
	}

	if (m_nextObjectIndex >= objectCount)
	{
		m_progress.MarkStageComplete(m_instantiateStage);
		Complete();
	}
}

void SceneLoadOperation::UpdateProgressAndStatus()
{
	if (m_phase == EPhase::Parsing)
	{
		// The parse job may still be writing its results; status text was set in OnStart.
		return;
	}

	const U32 requestedCount = m_resourceBatch.GetRequestedCount();
	const U32 cpuLoadedCount = m_resourceBatch.GetCPULoadedCount();
	const U32 settledCount = m_resourceBatch.GetSettledCount();

	m_progress.SetStage(m_cpuLoadStage, cpuLoadedCount, requestedCount);
	m_progress.SetStage(m_finalizeStage, settledCount, requestedCount);
	m_progress.SetStage(m_instantiateStage, m_nextObjectIndex, m_objectCount);

	SetProgress(m_progress.Compute());

	switch (m_phase)
	{
	case EPhase::Parsing:
		break;

	case EPhase::LoadingResources:
	{
		std::string status = cpuLoadedCount < requestedCount
			? "Loading assets (" + std::to_string(cpuLoadedCount) + "/" + std::to_string(requestedCount) + ")"
			: "Uploading to GPU (" + std::to_string(settledCount) + "/" + std::to_string(requestedCount) + ")";

		const std::string& lastPath = m_resourceBatch.GetLastProcessedPath();
		if (!lastPath.empty())
		{
			status += "\n" + lastPath;
		}

		SetStatusText(status);
		break;
	}

	case EPhase::Instantiating:
		SetStatusText("Creating objects (" + std::to_string(m_nextObjectIndex) + "/" + std::to_string(m_objectCount) + ")");
		break;
	}
}

void SceneLoadOperation::OnFinished(EAsyncOperationState finalState)
{
	if (finalState != EAsyncOperationState::Succeeded)
	{
		// Don't leave a half-built scene behind.
		m_scene.Unload();
		m_scene.SetCurrentScenePath(Filepath());
	}
}

void SceneLoadOperation::OnCleanup()
{
	// By now the components hold their own references, and any GPU uploads that read resource memory have
	// been processed, so the batch's references and any never-registered resources can safely go.
	m_resourceBatch.DiscardPending();
	m_resourceBatch.ReleaseHeldResources();
	m_document.reset();
}
