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

		// How wide the copied objects were, measured at Copy (see MeasureWidth)
		float GetContentsWidth() const { return m_contentsWidth; }

		// Counts a paste of the contents and returns how many offset steps to move it by: 1 for the first paste
		// since Copy, 2 for the second, and so on, so repeated pastes fan out instead of landing on each other
		int AdvancePasteSteps() { return ++m_pasteCount; }

		// Copies gameObjects into a scene-file string without touching the clipboard, e.g. for duplicating
		static std::string Serialize(const std::vector<GameObjectPtr>& gameObjects);

		// Metres along world X covered by gameObjects' meshes and their children's, taken together; 0 if none have
		// one. Measured from the originals, since a paste's meshes may still be loading.
		static float MeasureWidth(const std::vector<GameObjectPtr>& gameObjects);

	private:
		std::string m_contents;
		float m_contentsWidth = 0.0f;
		int m_pasteCount = 0;
	};
}
