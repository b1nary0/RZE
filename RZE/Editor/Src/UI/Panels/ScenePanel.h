#pragma once

#include <Utils/PrimitiveDefs.h>

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
			std::shared_ptr<GameObject> m_gameObject;
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
		std::shared_ptr<GameObject> GetSelectedGameObject();

	private:
		bool bEnabled;

		std::vector<std::string> mSelectedComponents;

		std::unique_ptr<SelectedItem> m_selectedItem;
	};
}