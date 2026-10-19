#include <StdAfx.h>
#include <Game/HeadlessScene.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>

#include <Game/GameScene.h>
#include <Game/SceneLoadOperation.h>
#include <Game/World/GameObject/GameObject.h>

#include <Graphics/RenderScene.h>
#include <Graphics/ViewCapture.h>

namespace
{
	// Its loads serve background work, which reports on them itself
	constexpr U32 k_operationFlags = EAsyncOperationFlags::Background | EAsyncOperationFlags::SubStep;
}

HeadlessScene::HeadlessScene()
	: m_scene(std::make_shared<GameScene>())
{
	m_scene->EnableLogging(false);
}

HeadlessScene::~HeadlessScene()
{
	Unload();
}

std::shared_ptr<AsyncOperation> HeadlessScene::Load(const Filepath& scenePath)
{
	CancelLoad();

	// The load replaces what's in the scene when it starts
	m_load = std::make_shared<SceneLoadOperation>(m_scene, scenePath, k_operationFlags);
	RZE().GetAsyncOperationManager().Start(m_load);
	return m_load;
}

std::shared_ptr<AsyncOperation> HeadlessScene::Preload(ResourceDependencyList resources)
{
	std::shared_ptr<AsyncOperation> preload = std::make_shared<ResourcePreloadOperation>(std::move(resources), k_operationFlags);
	RZE().GetAsyncOperationManager().Start(preload);
	m_preloads.push_back(preload);
	return preload;
}

bool HeadlessScene::IsLoading() const
{
	if (m_load != nullptr && !m_load->IsFinished())
	{
		return true;
	}

	for (const std::shared_ptr<AsyncOperation>& preload : m_preloads)
	{
		if (!preload->IsFinished())
		{
			return true;
		}
	}
	return false;
}

void HeadlessScene::ReleasePreloaded()
{
	for (const std::shared_ptr<AsyncOperation>& preload : m_preloads)
	{
		preload->RequestCancel();
	}
	m_preloads.clear();
}

GameObjectPtr HeadlessScene::AddGameObject(const std::string& name)
{
	return m_scene->AddGameObject(name);
}

void HeadlessScene::RemoveGameObject(GameObjectPtr& object)
{
	m_scene->RemoveGameObject(object);
	// Never updated, so nothing else would
	m_scene->ProcessObjectRemoveDeferrals();
}

std::shared_ptr<AsyncOperation> HeadlessScene::Capture(ViewCapture& capture, const RenderCamera& camera, float exposureCompensation, const Filepath& filepath)
{
	return capture.Capture(m_scene->GetRenderScene(), camera, exposureCompensation, filepath);
}

void HeadlessScene::Unload()
{
	CancelLoad();

	ReleasePreloaded();

	m_scene->Unload();
	m_scene->SetCurrentScenePath(Filepath());
	// Render commands hold their own references to what they use, so a capture queued this frame is unaffected
	m_scene->GetRenderScene().ReleaseStageState();
}

void HeadlessScene::CancelLoad()
{
	if (m_load != nullptr && !m_load->IsFinished())
	{
		m_load->RequestCancel();
		// A cancelled load empties its scene once its worker jobs are done, which could be after another load has
		// started filling it. So it keeps that scene to itself, and this one starts a new one.
		m_scene = std::make_shared<GameScene>();
	}
	m_load = nullptr;
}
