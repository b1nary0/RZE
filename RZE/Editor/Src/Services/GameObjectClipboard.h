#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <string>
#include <vector>

namespace Editor
{
	class EditorContext;

	// Copies GameObjects, with their components and children, in the scene file's format, and pastes them back as new
	// objects. Owned by EditorContext. Internal to the editor; nothing goes on the system clipboard.
	class GameObjectClipboard
	{
	public:
		GameObjectClipboard() = default;
		~GameObjectClipboard() = default;

	public:
		// Replaces the clipboard with gameObjects and their children. Objects already copied as some other object's
		// child, and objects that aren't saved with the scene (e.g. the editor camera), are left out.
		void Copy(const std::vector<GameObjectPtr>& gameObjects);
		bool HasContents() const { return !m_contents.empty(); }

		// A paste of the clipboard's contents, for PasteGameObjectsCommand; empty if there's nothing to paste
		const std::string& GetContents() const { return m_contents; }

		// Copies gameObjects into a scene-file string without touching the clipboard, e.g. for duplicating
		static std::string Serialize(const std::vector<GameObjectPtr>& gameObjects);

	private:
		std::string m_contents;
	};
}
