#include <Services/SceneDocument.h>

#include <Services/Thumbnails.h>
#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <Game/SceneLoadOperation.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	SceneDocument::SceneDocument(EditorContext& context)
		: m_context(context)
		, m_sceneModified(context.GetEvents().Subscribe<SceneModifiedEvent>([this](const SceneModifiedEvent&)
			{
				SetModified(!m_context.GetUndoHistory().IsAtSavePoint());
			}))
	{
	}

	void SceneDocument::New()
	{
		// An invalid path loads the default scene (SceneLoadOperation)
		Load(Filepath());
	}

	void SceneDocument::Load(const Filepath& filepath)
	{
		if (IsLoading())
		{
			RZE_LOG_WARNING("A scene is already loading.");
			return;
		}

		// The current scene is unloaded as soon as the operation starts, so this must come first
		m_context.ResetSceneState();

		const bool isNewScene = !filepath.IsValid();

		std::shared_ptr<SceneLoadOperation> loadOperation = std::make_shared<SceneLoadOperation>(RZE().GetActiveScene(), filepath);
		loadOperation->SetOnCompleted(AsyncOperation::CompletionCallback([this, isNewScene](const AsyncOperation& operation)
			{
				// Created last, so it ends up as the active camera over any camera in the scene
				CreateEditorCamera();

				// Whatever's in now is what the file has (or, after a failed load, there's nothing worth saving)
				SetModified(false);

				if (!operation.Succeeded())
				{
					// Nothing valid to save over
					m_isUntitled = true;
					RZE_LOG_ERROR("Failed to load scene: " << operation.GetErrorText());
					return;
				}

				m_isUntitled = isNewScene;

				m_context.GetEvents().Publish(SceneLoadedEvent{ *this });
			}));

		m_loadOperation = loadOperation;
		RZE().GetAsyncOperationManager().Start(loadOperation);
	}

	bool SceneDocument::IsLoading() const
	{
		return m_loadOperation != nullptr && !m_loadOperation->IsFinished();
	}

	const Filepath& SceneDocument::GetFilePath() const
	{
		return RZE().GetActiveScene().GetCurrentScenePath();
	}

	void SceneDocument::Save()
	{
		AssertMsg(HasFilePath(), "The scene has no file to save to; use SaveAs");

		// An invalid path saves to the scene's current one
		RZE().GetActiveScene().Serialize(Filepath());
		OnSaved();
	}

	void SceneDocument::SaveAs(const Filepath& filepath)
	{
		// Also makes filepath the scene's path for later Saves
		RZE().GetActiveScene().Serialize(filepath);
		m_isUntitled = false;
		OnSaved();
	}

	void SceneDocument::SaveWithPrompt(std::function<void(bool saved)> onDone)
	{
		if (HasFilePath())
		{
			Save();
			if (onDone)
			{
				onDone(true);
			}
			return;
		}

		FileBrowserRequest request;
		request.Mode = EFileBrowserMode::Save;
		request.Title = "Save Scene";
		request.ConfirmLabel = "Save";
		request.RootFolder = AssetPaths::GetSceneFolder();
		request.Extensions = { AssetPaths::k_sceneExtension };
		request.ThumbnailFor = [](const Filepath& file) { return Thumbnails::Scenes().GetPathFor(file); };
		m_context.GetThumbnails().AddToBrowser(request);
		request.OnChosen = [this, onDone](const Filepath& file)
			{
				SaveAs(file);
				if (onDone)
				{
					onDone(true);
				}
			};
		request.OnCancelled = [onDone]()
			{
				if (onDone)
				{
					onDone(false);
				}
			};
		m_context.GetFileBrowser().Open(std::move(request));
	}

	void SceneDocument::OnSaved()
	{
		m_context.GetUndoHistory().MarkSaved();
		SetModified(false);
		m_context.GetEvents().Publish(SceneSavedEvent{ *this });
	}

	void SceneDocument::SetModified(bool isModified)
	{
		if (m_isModified != isModified)
		{
			m_isModified = isModified;
			m_context.GetEvents().Publish(SceneModifiedStateChangedEvent{ *this });
		}
	}

	void SceneDocument::CreateEditorCamera()
	{
		// Setting up the editor rather than editing the scene, so not a command
		const CommandQueue::SceneEditScope sceneEdit(m_context.GetCommands());
		GameObjectPtr editorCameraObject = RZE().GetActiveScene().AddGameObject("EditorCam");
		GameObjectComponentPtr<TransformComponent> transformComponent = editorCameraObject->GetComponent<TransformComponent>();
		transformComponent->SetPosition(Vector3D(-4.0f, 10.0f, 4.0f));

		GameObjectComponentPtr<EditorCameraComponent> editorCam = editorCameraObject->AddComponent<EditorCameraComponent>();
		editorCam->SetAsActiveCamera(true);
		const Vector2D& viewportSize = m_context.GetSceneViewport().Size;
		editorCam->SetAspectRatio(viewportSize.X() / viewportSize.Y());

		m_context.SetEditorCamera(editorCameraObject);

		// The editor camera drives the scene view; the scene's main camera is only for the game
		RZE().GetActiveScene().SetMainCameraDrivesView(false);
	}
}
