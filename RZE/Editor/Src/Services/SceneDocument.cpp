#include <Services/SceneDocument.h>

#include <UI/Framework/EditorContext.h>

#include <RZE.h>

#include <Game/SceneLoadOperation.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	SceneDocument::SceneDocument(EditorContext& context)
		: m_context(context)
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
			DebugServices::Get().Trace(LogChannel::Info, "A scene is already loading.");
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

				if (!operation.Succeeded())
				{
					// Nothing valid to save over
					m_isUntitled = true;
					DebugServices::Get().Trace(LogChannel::Info, "Failed to load scene: " + operation.GetErrorText());
					return;
				}

				m_isUntitled = isNewScene;

				if (m_onLoaded != nullptr)
				{
					m_onLoaded(*this);
				}
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

	void SceneDocument::OnSaved()
	{
		m_context.GetGameBuilder().CopyAssets();
	}

	void SceneDocument::CreateEditorCamera()
	{
		GameObjectPtr editorCameraObject = RZE().GetActiveScene().AddGameObject("EditorCam");
		GameObjectComponentPtr<TransformComponent> transformComponent = editorCameraObject->GetComponent<TransformComponent>();
		transformComponent->SetPosition(Vector3D(-4.0f, 10.0f, 4.0f));

		GameObjectComponentPtr<EditorCameraComponent> editorCam = editorCameraObject->AddComponent<EditorCameraComponent>();
		editorCam->SetAsActiveCamera(true);
		const Vector2D& viewportSize = m_context.GetSceneViewport().Size;
		editorCam->SetAspectRatio(viewportSize.X() / viewportSize.Y());

		m_context.SetEditorCamera(editorCameraObject);
	}
}
