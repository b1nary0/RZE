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
		struct SelectedItem
		{
			GameObjectPtr m_gameObject;
		};
	public:
		ScenePanel();
		~ScenePanel();

	public:
		void Display();

		void Enable();
		void Disable();
		bool IsEnabled();

		bool HasSelectedGameObject();
		GameObjectPtr GetSelectedGameObject();
		void ResetSelectedGameObject();

	private:
		bool bEnabled;

		std::vector<std::string> mSelectedComponents;

		std::unique_ptr<SelectedItem> m_selectedItem;
	};
}