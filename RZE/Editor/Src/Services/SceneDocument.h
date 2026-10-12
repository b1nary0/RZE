#pragma once

#include <Utils/Functor.h>
#include <Utils/Platform/Filepath.h>

#include <memory>

class AsyncOperation;

namespace Editor
{
	class EditorContext;

	// The scene being edited: creating, loading and saving it. Owned by EditorContext.
	class SceneDocument
	{
	public:
		using LoadedCallback = Functor<void, const SceneDocument&>;

	public:
		explicit SceneDocument(EditorContext& context);
		~SceneDocument() = default;

		SceneDocument(const SceneDocument&) = delete;
		SceneDocument& operator=(const SceneDocument&) = delete;

	public:
		// Called each time a load (or New) succeeds, once the scene and the editor camera are in
		void SetOnLoaded(const LoadedCallback& onLoaded) { m_onLoaded = onLoaded; }

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

	private:
		void CreateEditorCamera();
		// Copies the saved scene to where the game reads it
		void OnSaved();

	private:
		EditorContext& m_context;
		std::shared_ptr<AsyncOperation> m_loadOperation;
		LoadedCallback m_onLoaded;

		// A new scene is loaded from the default scene's file and keeps its path, so the scene's path alone can't
		// tell whether saving would overwrite the default scene
		bool m_isUntitled = true;
	};
}
