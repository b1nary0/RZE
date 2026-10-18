#pragma once

#include <Commands/EditorCommand.h>

#include <Utils/Platform/Filepath.h>

// Undoable changes to the project's files, through AssetFiles: they keep thumbnails in step, and deleting goes through
// the editor's trash. They don't change the scene (UndoRecord::ChangesScene), so they stay in the history when another
// scene loads. Each one, and its undo and redo, publishes AssetFileChangedEvent. When the files have changed since in a
// way that stops one (e.g. something else has the name now), it does nothing and says why in the Log.
//
// None of them touches the open scene's file, or a folder it's in (AssetFiles::GetWhyInUse).
namespace Editor
{
	// Renames a file or folder in its folder, e.g. to a name from FileNames::ResolveRename
	class RenameAssetCommand : public EditorCommand
	{
	public:
		RenameAssetCommand(const Filepath& from, const Filepath& to);

		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Rename Asset"; }

	private:
		Filepath m_from;
		Filepath m_to;
	};

	// Moves a file or folder into the trash. Undo puts it back, and it goes on to the Recycle Bin once the history
	// drops the step.
	class DeleteAssetCommand : public EditorCommand
	{
	public:
		explicit DeleteAssetCommand(const Filepath& path);

		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Delete Asset"; }

	private:
		Filepath m_path;
	};

	// Makes a folder, which mustn't exist yet (e.g. a name from FileNames::MakeUnique). Undo removes it if it's still
	// empty.
	class CreateFolderCommand : public EditorCommand
	{
	public:
		explicit CreateFolderCommand(const Filepath& folder);

		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Create Folder"; }

	private:
		Filepath m_folder;
	};
}
