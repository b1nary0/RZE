#pragma once

#include <UI/Framework/EditorPanel.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

namespace Editor
{
	// The scene's GameObject hierarchy: selection, reparenting by drag and drop, and per-object actions
	// (rename, add component, detach, delete) from the context menu
	class ScenePanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;
		void OnSceneReset() override;

		void DisplayObject(EditorContext& context, GameObjectPtr gameObject);

	private:
		// Whether the name prompt for a new GameObject is showing
		bool m_isCreatingGameObject = false;
	};
}
