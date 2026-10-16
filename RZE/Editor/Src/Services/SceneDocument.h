#pragma once

#include <UI/Framework/EditorEvents.h>

#include <Utils/Platform/Filepath.h>

#include <functional>
#include <memory>

class AsyncOperation;

namespace Editor
{
	class EditorContext;

	// The scene being edited: creating, loading and saving it, and whether it has unsaved changes. Owned by
	// EditorContext. Publishes SceneLoadedEvent each time a load (or New) succeeds, SceneSavedEvent after each save,
	// and SceneModifiedStateChangedEvent when IsModified changes.
	class SceneDocument
	{
	public:
		explicit SceneDocument(EditorContext& context);
		~SceneDocument() = default;

		SceneDocument(const SceneDocument&) = delete;
		SceneDocument& operator=(const SceneDocument&) = delete;

	public:
		// Replaces the scene with a copy of the default one, which has no file of its own until it's saved with SaveAs
		void New();
		// Asynchronous: the current scene is unloaded straight away and the new one streams in over the following
		// frames. An invalid filepath is the same as New. Ignored (with a log line) while another load is running.
		void Load(const Filepath& filepath);
		bool IsLoading() const;

		// False for a new scene, or one that failed to load; Save needs a file, so use SaveAs
		bool HasFilePath() const { return !m_isUntitled; }
		// The file Save writes to. Only meaningful while HasFilePath() is true.
		const Filepath& GetFilePath() const;

		void Save();
		void SaveAs(const Filepath& filepath);
		// Saves to the scene's file, or asks for one in the file browser first if it hasn't got one. onDone, if given,
		// runs once: with true when the scene is saved (straight away if it has a file), or false if the browser is
		// cancelled.
		void SaveWithPrompt(std::function<void(bool saved)> onDone = nullptr);

		// Differs from its file: the undo history isn't at the point of the last load or save, so undoing back to
		// it clears this (UndoHistory::IsAtSavePoint). Updated after each command flush (SceneModifiedEvent).
		bool IsModified() const { return m_isModified; }

	private:
		void CreateEditorCamera();
		void SetModified(bool isModified);
		// Marks the undo history saved and publishes SceneSavedEvent
		void OnSaved();

	private:
		EditorContext& m_context;
		std::shared_ptr<AsyncOperation> m_loadOperation;

		// A new scene is loaded from the default scene's file and keeps its path, so the scene's path alone can't
		// tell whether saving would overwrite the default scene
		bool m_isUntitled = true;
		bool m_isModified = false;

		EventSubscription m_sceneModified;
	};
}
