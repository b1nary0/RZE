#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <memory>
#include <string>
#include <vector>

class GameObject;

namespace Editor
{
	class ScenePanel
	{
	public:
		ScenePanel();
		~ScenePanel();

	public:
		void Display();

		void Enable();
		void Disable();
		bool IsEnabled();

		bool HasSelectedGameObject();
		// The primary selection: the most recently selected object, which the gizmo and inspector act on
		GameObjectPtr GetSelectedGameObject();
		bool IsSelected(const GameObjectPtr& gameObject) const;
		// Clears the whole selection
		void ResetSelectedGameObject();

		// Replaces the selection
		void SelectGameObject(GameObjectPtr gameObject);
		void SetSelection(const std::vector<GameObjectPtr>& gameObjects);
		// Ctrl+click: adds the object, or removes it if it's already selected
		void ToggleSelected(GameObjectPtr gameObject);
		void AddToSelection(const std::vector<GameObjectPtr>& gameObjects);

	private:
		void DisplayObject(GameObjectPtr gameObject);
		void AddSelected(GameObjectPtr gameObject);

	private:
		bool bEnabled;

		std::vector<std::string> mSelectedComponents;

		// In selection order; the last entry is the primary
		std::vector<GameObjectPtr> m_selection;

		// The object m_components was cached for
		GameObjectPtr m_inspectedObject;
		std::vector<GameObjectComponentBase*> m_components;
	};
}