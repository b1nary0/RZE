#include <UI/Framework/EditorContext.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	void EditorContext::Update()
	{
		m_scenePicker.Update();
	}

	void EditorContext::ResetSceneState()
	{
		m_selection.Clear();
		m_editorCamera = GameObjectPtr();
		m_scenePicker.Cancel();
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
