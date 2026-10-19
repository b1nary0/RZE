#include <Services/ThumbnailRenderer.h>

#include <Services/MeshFraming.h>
#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Modals/FileBrowserModal.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/PendingResource.h>

#include <Game/CaptureBatchOperation.h>
#include <Game/GameScene.h>
#include <Game/StaticMeshResource.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/StringUtils.h>

#include <algorithm>

namespace
{
	// Narrow, like a product shot, so a mesh doesn't look stretched at the edges
	constexpr float k_meshFieldOfView = 40.0f;
	// For a scene with no camera of its own, framed from its bounds
	constexpr float k_sceneFieldOfView = 50.0f;
}

namespace Editor
{
	namespace
	{
		// Works through ThumbnailRenderer's queue, in its headless scene, until there's nothing left. What it works with
		// belongs to the renderer, as it outlives each run.
		class ThumbnailRenderJob final : public CaptureBatchOperation
		{
		public:
			ThumbnailRenderJob(ThumbnailQueue& queue, HeadlessScene& scene, ViewCapture& capture, EditorEvents& events)
				: CaptureBatchOperation("Rendering thumbnails", EAsyncOperationFlags::Background)
				, m_queue(queue)
				, m_scene(scene)
				, m_capture(capture)
				, m_events(events)
			{
			}

		protected:
			bool TakeNextItem(Filepath& outItem) override
			{
				return m_queue.TakeNext(outItem);
			}

			size_t GetItemsLeft() const override
			{
				return m_queue.GetQueuedCount();
			}

			void BeginItem(const Filepath& item) override
			{
				HeadlessScene& scene = m_scene;
				m_item = item;
				m_load = nullptr;
				m_mesh.Reset();

				if (AssetPaths::IsScene(item))
				{
					m_load = scene.Load(item);
					m_isMeshStageLoaded = false;
					return;
				}

				// Meshes in a row share the stage: Default.scene's camera and light
				if (!m_isMeshStageLoaded)
				{
					m_load = scene.Load(GameScene::GetDefaultScenePath());
					m_isMeshStageLoaded = true;
				}
				// A step of this run, which reports on it
				m_mesh.Load<StaticMeshResource>(item, EAsyncOperationFlags::Background | EAsyncOperationFlags::SubStep);
			}

			bool IsItemReady() override
			{
				if (m_load != nullptr)
				{
					if (!m_load->IsFinished())
					{
						return false;
					}
					if (!m_load->Succeeded())
					{
						FailItem(m_load->GetErrorText().empty() ? "it couldn't be loaded" : m_load->GetErrorText());
						return false;
					}
				}

				if (AssetPaths::IsScene(m_item) || m_meshObject != nullptr)
				{
					return true;
				}
				if (m_mesh.IsLoading())
				{
					return false;
				}
				if (m_mesh.HasFailed())
				{
					FailItem("it couldn't be loaded");
					return false;
				}

				// Loaded, so this is a cache hit rather than a load on the main thread
				HeadlessScene& scene = m_scene;
				m_meshObject = scene.AddGameObject("ThumbnailMesh");
				m_meshComponent = m_meshObject->AddComponent<RenderComponent>();
				m_meshComponent->LoadMesh(m_item);
				return true;
			}

			void CaptureItem(const Filepath& item, std::function<void(bool succeeded, const std::string& error)> onDone) override
			{
				ViewCapture& capture = m_capture;
				RenderCamera camera;
				float exposureCompensation = 0.0f;

				if (AssetPaths::IsMesh(item))
				{
					const BoundingBox bounds = m_meshComponent->GetWorldBounds();
					if (bounds.IsEmpty())
					{
						onDone(false, "the mesh has nothing to draw");
						return;
					}
					camera = MeshFraming::FrameBounds(bounds, k_meshFieldOfView, capture.GetSize());
				}
				else
				{
					GameScene& scene = m_scene.GetScene();
					const GameObjectPtr mainCamera = scene.GetMainCamera();
					if (mainCamera != nullptr)
					{
						GameObjectComponentPtr<CameraComponent> cameraComponent = mainCamera->GetComponent<CameraComponent>();
						camera = cameraComponent->CreateRenderCamera(capture.GetSize());
						exposureCompensation = cameraComponent->GetExposureCompensation();
					}
					else if (!scene.GetBounds().IsEmpty())
					{
						camera = MeshFraming::FrameBounds(scene.GetBounds(), k_sceneFieldOfView, capture.GetSize());
					}
					else
					{
						onDone(false, "it has no camera and nothing to frame");
						return;
					}
				}

				std::shared_ptr<AsyncOperation> written = m_scene.Capture(capture, camera, exposureCompensation, Thumbnails::FindFor(item));
				written->SetOnCompleted(AsyncOperation::CompletionCallback([onDone](const AsyncOperation& operation)
					{
						onDone(operation.Succeeded(), operation.GetErrorText());
					}));
			}

			void EndItem(const Filepath& item, bool succeeded) override
			{
				m_queue.FinishCurrent();

				if (m_meshObject != nullptr)
				{
					m_meshComponent = nullptr;
					m_scene.RemoveGameObject(m_meshObject);
				}
				// The object held the mesh while it was needed
				m_mesh.Reset();

				if (succeeded)
				{
					m_events.Publish(ThumbnailWrittenEvent{ Thumbnails::FindFor(item) });
				}
			}

			void EndBatch(EAsyncOperationState finalState) override
			{
				const size_t skippedCount = m_queue.TakeSkippedCount();
				if (skippedCount > 0)
				{
					LogStep(std::to_string(skippedCount) + " already up to date");
				}

				if (finalState == EAsyncOperationState::Cancelled)
				{
					m_queue.Clear();
				}

				// Frees what it loaded until it's needed again
				m_mesh.Reset();
				m_scene.Unload();
				m_isMeshStageLoaded = false;
			}

		private:
			ThumbnailQueue& m_queue;
			HeadlessScene& m_scene;
			ViewCapture& m_capture;
			EditorEvents& m_events;

			// What the item waits for: the scene (an item's own, or the stage for meshes), and the mesh
			std::shared_ptr<AsyncOperation> m_load;
			PendingResource m_mesh;
			// Whether the headless scene holds Default.scene, for staging meshes
			bool m_isMeshStageLoaded = false;

			GameObjectPtr m_meshObject;
			GameObjectComponentPtr<RenderComponent> m_meshComponent;

			// For IsItemReady, which isn't given it
			Filepath m_item;
		};
	}

	ThumbnailRenderer::ThumbnailRenderer(EditorContext& context)
		: m_context(context)
		, m_capture(Thumbnails::k_size, Thumbnails::k_size)
	{
		EditorEvents& events = context.GetEvents();
		m_sceneSaved = events.Subscribe<SceneSavedEvent>([this](const SceneSavedEvent& event)
			{
				if (event.Document.HasFilePath())
				{
					Request(event.Document.GetFilePath(), true);
				}
			});
		m_sceneLoaded = events.Subscribe<SceneLoadedEvent>([this](const SceneLoadedEvent& event)
			{
				if (event.Document.HasFilePath())
				{
					Request(event.Document.GetFilePath());
				}
			});
	}

	ThumbnailRenderer::~ThumbnailRenderer()
	{
		ShutDown();
	}

	void ThumbnailRenderer::Request(const Filepath& asset, bool force)
	{
		if (m_isShutDown || !Thumbnails::IsRendered(asset))
		{
			return;
		}

		// Checked here too, so a thumbnail that's up to date doesn't start a run that then has nothing to do
		if (!force && Thumbnails::IsUpToDate(asset, Thumbnails::FindFor(asset)))
		{
			return;
		}

		m_queue.Add(asset, force);
		StartIfNeeded();
	}

	void ThumbnailRenderer::AddToBrowser(FileBrowserRequest& request)
	{
		const bool listsRenderedFiles = !request.Extensions.empty() && std::all_of(request.Extensions.begin(), request.Extensions.end(),
			[](const std::string& extension)
			{
				return StringUtils::EqualsIgnoringCase(extension, AssetPaths::k_sceneExtension) || StringUtils::EqualsIgnoringCase(extension, AssetPaths::k_meshExtension);
			});
		if (!listsRenderedFiles)
		{
			return;
		}

		request.OnThumbnailMissing = [this](const Filepath& file) { Request(file); };
		request.TileActions.push_back({ "Refresh Thumbnail", [this](const Filepath& file) { Request(file, true); }, &Thumbnails::IsRendered });
	}

	void ThumbnailRenderer::ShutDown()
	{
		if (m_isShutDown)
		{
			return;
		}
		m_isShutDown = true;

		if (m_job != nullptr)
		{
			m_job->RequestCancel();
			m_job = nullptr;
		}
		m_queue.Clear();
		m_scene.Unload();
	}

	void ThumbnailRenderer::StartIfNeeded()
	{
		// A run that's going takes what's been added; one that's finishing starts the next when it's done
		if (m_job != nullptr || m_queue.GetQueuedCount() == 0)
		{
			return;
		}

		m_job = std::make_shared<ThumbnailRenderJob>(m_queue, m_scene, m_capture, m_context.GetEvents());
		m_job->SetOnCompleted(AsyncOperation::CompletionCallback([this](const AsyncOperation&)
			{
				m_job = nullptr;
				StartIfNeeded();
			}));
		RZE().GetAsyncOperationManager().Start(m_job);
	}
}
