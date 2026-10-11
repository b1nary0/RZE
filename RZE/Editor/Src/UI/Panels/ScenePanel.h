#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <memory>
#include <string>
#include <vector>

class GameObject;

namespace Editor
{
	class EditorContext;

	class ScenePanel
	{
	public:
		ScenePanel();
		~ScenePanel();

	public:
		void Display(EditorContext& context);

		void Enable();
		void Disable();
		bool IsEnabled();

	private:
		void DisplayObject(EditorContext& context, GameObjectPtr gameObject);

	private:
		bool bEnabled;

		std::vector<std::string> mSelectedComponents;

		// The object m_components was cached for
		GameObjectPtr m_inspectedObject;
		std::vector<GameObjectComponentBase*> m_components;
	};
}