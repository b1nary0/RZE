#include <Services/SceneThumbnailCapturer.h>

#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperation.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <DebugUtils/DebugServices.h>

namespace
{
	// How long a loaded scene settles before it's captured: its first frames build the shadow map and finish
	// uploading what it streamed in
	constexpr int k_settleFrames = 10;
}

namespace Editor
{
	SceneThumbnailCapturer::SceneThumbnailCapturer(EditorContext& context)
		: m_context(context)
		, m_capture(Thumbnails::k_size, Thumbnails::k_size)
	{
		EditorEvents& events = context.GetEvents();
		m_sceneSaved = events.Subscribe<SceneSavedEvent>([this](const SceneSavedEvent&)
			{
				CaptureNow();
			});
		m_sceneLoaded = events.Subscribe<SceneLoadedEvent>([this](const SceneLoadedEvent& event)
			{
				if (event.Document.HasFilePath() && !Thumbnails::Scenes().GetPathFor(event.Document.GetFilePath()).Exists())
				{
					m_framesUntilCapture = k_settleFrames;
				}
			});
		m_sceneUnloading = events.Subscribe<SceneUnloadingEvent>([this](const SceneUnloadingEvent&)
			{
				m_framesUntilCapture = -1;
			});
	}

	void SceneThumbnailCapturer::Update()
	{
		if (m_framesUntilCapture < 0)
		{
			return;
		}

		if (m_framesUntilCapture-- == 0)
		{
			CaptureNow();
		}
	}

	void SceneThumbnailCapturer::CaptureNow(DoneCallback onDone)
	{
		const SceneDocument& document = m_context.GetSceneDocument();
		if (!document.HasFilePath() || document.IsLoading())
		{
			if (onDone)
			{
				onDone(false);
			}
			return;
		}

		const Filepath thumbnailPath = Thumbnails::Scenes().GetPathFor(document.GetFilePath());
		const Vector2D& size = m_capture.GetSize();

		RenderCamera camera;
		float exposureCompensation = 0.0f;

		GameObjectPtr mainCamera = RZE().GetActiveScene().GetMainCamera();
		GameObjectComponentPtr<EditorCameraComponent> editorCamera = m_context.GetEditorCameraComponent();
		if (mainCamera != nullptr)
		{
			GameObjectComponentPtr<CameraComponent> cameraComponent = mainCamera->GetComponent<CameraComponent>();
			camera = cameraComponent->CreateRenderCamera(size);
			exposureCompensation = cameraComponent->GetExposureCompensation();
		}
		else if (editorCamera != nullptr)
		{
			// The editor camera's view, at the capture's aspect ratio
			const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(editorCamera->GetFOV(), size.X() / size.Y(),
				editorCamera->GetNearCull(), editorCamera->GetFarCull());
			camera.ClipSpace = projection * editorCamera->GetViewMatrix();
			camera.Position = m_context.GetEditorCamera()->GetTransformComponent()->GetPosition();
		}
		else
		{
			if (onDone)
			{
				onDone(false);
			}
			return;
		}

		std::shared_ptr<AsyncOperation> operation = m_capture.Capture(camera, exposureCompensation, thumbnailPath);
		operation->SetOnCompleted(AsyncOperation::CompletionCallback([onDone](const AsyncOperation& operation)
			{
				if (!operation.Succeeded())
				{
					DebugServices::Get().Trace(LogChannel::Info, "Couldn't save the scene's thumbnail: " + operation.GetErrorText());
				}

				if (onDone)
				{
					onDone(operation.Succeeded());
				}
			}));
	}
}
