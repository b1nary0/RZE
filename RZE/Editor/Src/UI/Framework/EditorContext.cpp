#include <UI/Framework/EditorContext.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	EditorContext::EditorContext()
		: m_selection(m_events)
		, m_selectionBounds(m_events)
		, m_sceneDocument(*this)
		, m_thumbnails(*this)
	{
		m_thumbnailWritten = m_events.Subscribe<ThumbnailWrittenEvent>([this](const ThumbnailWrittenEvent& event)
			{
				m_fileBrowser.RefreshThumbnail(event.Image);
			});
	}

	void EditorContext::Update()
	{
		m_sceneViewport.IsHovered = false;
		m_scenePicker.Update();
	}

	void EditorContext::ResetSceneState()
	{
		m_selection.Clear();
		m_editorCamera = GameObjectPtr();
		m_scenePicker.Cancel();
		m_commands.Clear();
		m_undoHistory.Clear();
		m_events.Publish(SceneUnloadingEvent{});
	}

	GameObjectComponentPtr<EditorCameraComponent> EditorContext::GetEditorCameraComponent() const
	{
		if (m_editorCamera == nullptr)
		{
			return nullptr;
		}

		GameObjectComponentPtr<EditorCameraComponent> cameraComponent = m_editorCamera->GetComponent<EditorCameraComponent>();
		AssertNotNull(cameraComponent);
		return cameraComponent;
	}
}
