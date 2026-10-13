#include <Services/ThumbnailBatchOperation.h>

#include <Services/FolderListing.h>
#include <Services/MeshFraming.h>
#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/ViewCapture.h>

#include <DebugUtils/DebugServices.h>

namespace
{
	// Narrow, like a product shot, so a mesh doesn't look stretched at the edges
	constexpr float k_meshFieldOfView = 40.0f;
	// Meshes load with the item, so they only need their uploads and first transform
	constexpr int k_meshSettleFrames = 3;

	// The files with one of extensions in folder, not counting subfolders
	std::vector<Filepath> ListFiles(const Filepath& folder, const std::vector<std::string>& extensions)
	{
		std::vector<Filepath> files;
		for (const Editor::FolderEntry& entry : Editor::ListFolder(folder, extensions))
		{
			if (!entry.IsFolder)
			{
				files.push_back(entry.Path);
			}
		}
		return files;
	}

	// Loads each scene, then captures it through its main camera
	class SceneThumbnailBatch final : public Editor::ThumbnailBatchOperation
	{
	public:
		SceneThumbnailBatch(Editor::EditorContext& context, std::vector<Filepath> scenes)
			: ThumbnailBatchOperation(context, "Rendering scene thumbnails", std::move(scenes))
		{
		}

	protected:
		void BeginItem(const Filepath& item) override
		{
			GetContext().GetSceneDocument().Load(item);
		}

		bool IsItemReady() override
		{
			return !GetContext().GetSceneDocument().IsLoading();
		}

		void CaptureItem(const Filepath& item, std::function<void(bool succeeded)> onDone) override
		{
			// A scene that failed to load has no file, so this reports the failure
			GetContext().GetSceneThumbnails().CaptureNow(std::move(onDone));
		}
	};

	// Puts each mesh on its own in a new scene, then captures it framed from a three-quarter angle
	class MeshThumbnailBatch final : public Editor::ThumbnailBatchOperation
	{
	public:
		MeshThumbnailBatch(Editor::EditorContext& context, std::vector<Filepath> meshes)
			: ThumbnailBatchOperation(context, "Rendering mesh thumbnails", std::move(meshes))
			, m_capture(Editor::Thumbnails::k_size, Editor::Thumbnails::k_size)
		{
		}

	protected:
		void BeginBatch() override
		{
			// Only a camera and a light, and captures leave out the grid
			GetContext().GetSceneDocument().New();
		}

		void BeginItem(const Filepath& item) override
		{
			// Staging the thumbnail rather than editing the scene, so not a command
			const Editor::CommandQueue::SceneEditScope sceneEdit(GetContext().GetCommands());
			m_object = RZE().GetActiveScene().AddGameObject("ThumbnailMesh");
			m_renderComponent = m_object->AddComponent<RenderComponent>();
			m_renderComponent->LoadMesh(item);
		}

		bool IsItemReady() override
		{
			return m_renderComponent != nullptr && m_renderComponent->GetRenderObject() != nullptr;
		}

		int GetSettleFrames() const override
		{
			return k_meshSettleFrames;
		}

		void CaptureItem(const Filepath& item, std::function<void(bool succeeded)> onDone) override
		{
			Vector3D boundsMin;
			Vector3D boundsMax;
			if (!m_renderComponent->GetWorldBounds(boundsMin, boundsMax))
			{
				onDone(false);
				return;
			}

			const RenderCamera camera = Editor::MeshFraming::FrameBounds(boundsMin, boundsMax, k_meshFieldOfView, m_capture.GetSize());
			std::shared_ptr<AsyncOperation> capture = m_capture.Capture(RZE().GetRenderEngine().GetMainScene(), camera, 0.0f, Editor::Thumbnails::Meshes().GetPathFor(item));
			capture->SetOnCompleted(AsyncOperation::CompletionCallback([onDone](const AsyncOperation& operation)
				{
					onDone(operation.Succeeded());
				}));
		}

		void EndItem() override
		{
			const Editor::CommandQueue::SceneEditScope sceneEdit(GetContext().GetCommands());
			m_renderComponent = nullptr;
			RZE().GetActiveScene().RemoveGameObject(m_object);
		}

	private:
		ViewCapture m_capture;
		GameObjectPtr m_object;
		GameObjectComponentPtr<RenderComponent> m_renderComponent;
	};

	// Starts batch once any unsaved changes are dealt with, and shows the new thumbnails in the browser when it's done
	void StartBatch(Editor::EditorContext& context, std::shared_ptr<Editor::ThumbnailBatchOperation> batch)
	{
		context.GetUnsavedChangesPrompt().Request(context, "rendering thumbnails", [&context, batch]()
			{
				batch->SetOnCompleted(AsyncOperation::CompletionCallback([&context](const AsyncOperation&)
					{
						context.GetFileBrowser().RefreshThumbnails();
					}));
				RZE().GetAsyncOperationManager().Start(batch);
			});
	}
}

namespace Editor
{
	ThumbnailBatchOperation::ThumbnailBatchOperation(EditorContext& context, const std::string& name, std::vector<Filepath> items)
		: AsyncOperation(name, EAsyncOperationFlags::Blocking)
		, m_context(context)
		, m_items(std::move(items))
	{
	}

	void ThumbnailBatchOperation::OnStart()
	{
		const SceneDocument& document = m_context.GetSceneDocument();
		m_originalScene = document.HasFilePath() ? document.GetFilePath() : Filepath();

		m_context.GetSceneThumbnails().SetCapturesOnLoad(false);
		BeginBatch();
		m_state = EState::Preparing;
	}

	void ThumbnailBatchOperation::OnTick(AsyncFrameBudget& budget)
	{
		const SceneDocument& document = m_context.GetSceneDocument();

		switch (m_state)
		{
		case EState::Preparing:
		{
			if (!document.IsLoading())
			{
				StartNextItem();
			}
			break;
		}

		case EState::WaitingForItem:
		{
			if (IsItemReady())
			{
				m_settleFramesLeft = GetSettleFrames();
				m_state = EState::Settling;
			}
			break;
		}

		case EState::Settling:
		{
			if (m_settleFramesLeft-- > 0)
			{
				break;
			}

			m_captureResult = std::make_shared<CaptureResult>();
			std::shared_ptr<CaptureResult> result = m_captureResult;
			CaptureItem(m_items[m_itemIndex], [result](bool succeeded)
				{
					result->IsDone = true;
					result->Succeeded = succeeded;
				});
			m_state = EState::Capturing;
			break;
		}

		case EState::Capturing:
		{
			if (!m_captureResult->IsDone)
			{
				break;
			}

			if (!m_captureResult->Succeeded)
			{
				++m_failedCount;
				DebugServices::Get().Trace(LogChannel::Info, "Couldn't render the thumbnail of " + m_items[m_itemIndex].GetRelativePath());
			}

			EndItem();
			++m_itemIndex;
			StartNextItem();
			break;
		}

		case EState::Restoring:
		{
			if (!document.IsLoading())
			{
				Complete();
			}
			break;
		}
		}
	}

	void ThumbnailBatchOperation::OnFinished(EAsyncOperationState finalState)
	{
		m_context.GetSceneThumbnails().SetCapturesOnLoad(true);

		if (m_failedCount > 0)
		{
			DebugServices::Get().Trace(LogChannel::Info, GetName() + ": " + std::to_string(m_failedCount) + " of "
				+ std::to_string(m_items.size()) + " couldn't be rendered");
		}
	}

	void ThumbnailBatchOperation::StartNextItem()
	{
		SetProgress(m_items.empty() ? 1.0f : static_cast<float>(m_itemIndex) / static_cast<float>(m_items.size()));

		if (m_itemIndex >= m_items.size())
		{
			SetStatusText("Loading the scene again");
			if (m_originalScene.IsValid())
			{
				m_context.GetSceneDocument().Load(m_originalScene);
			}
			else
			{
				m_context.GetSceneDocument().New();
			}
			m_state = EState::Restoring;
			return;
		}

		const Filepath& item = m_items[m_itemIndex];
		SetStatusText(item.GetStem() + " (" + std::to_string(m_itemIndex + 1) + " of " + std::to_string(m_items.size()) + ")");
		BeginItem(item);
		m_state = EState::WaitingForItem;
	}

	namespace ThumbnailBatches
	{
		void RenderScenes(EditorContext& context, std::vector<Filepath> scenes)
		{
			if (scenes.empty())
			{
				scenes = ListFiles(AssetPaths::GetSceneFolder(), { AssetPaths::k_sceneExtension });
			}
			StartBatch(context, std::make_shared<SceneThumbnailBatch>(context, std::move(scenes)));
		}

		void RenderMeshes(EditorContext& context)
		{
			StartBatch(context, std::make_shared<MeshThumbnailBatch>(context, ListFiles(AssetPaths::GetMeshFolder(), { AssetPaths::k_meshExtension })));
		}
	}
}
