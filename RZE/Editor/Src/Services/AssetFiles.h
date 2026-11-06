#pragma once

#include <Utils/Platform/Filepath.h>

#include <string>
#include <vector>

namespace Editor
{
	// A file or folder MoveToTrash set aside, with the thumbnails that went with it
	struct TrashedItem
	{
		// Where it was
		Filepath Original;
		// Where it is now; invalid if it couldn't be moved
		Filepath InTrash;

		struct Thumbnail
		{
			Filepath Original;
			Filepath InTrash;
		};
		std::vector<Thumbnail> Thumbnails;

		bool IsInTrash() const { return InTrash.IsValid(); }
	};

	// Changes to the project's files that keep the editor's own files beside them in step: the thumbnails rendered for
	// scenes and meshes (Thumbnails). Nothing that refers to a file by its path, like a scene using a mesh or a mesh
	// using a material, is updated.
	//
	// Deleting goes through the editor's trash (ProjectData/.Trash), so it can be undone, and on to the Recycle Bin
	// once it can't. It goes to the Recycle Bin from where it was, so restoring it there puts it back in the project.
	namespace AssetFiles
	{
		// Moves a file or folder to the Recycle Bin, as Shell::MoveToRecycleBin does, which tests stand in for. False if
		// it couldn't.
		using SendToRecycleBin = bool (*)(const Filepath& path);

		// Renames a file or folder to to, in the same folder (File::Move), and a scene's or mesh's thumbnail with it.
		// False if it couldn't, and nothing changes then.
		bool Rename(const Filepath& from, const Filepath& to);

		// Makes folder, which mustn't exist yet. False if it couldn't.
		bool CreateFolder(const Filepath& folder);

		// Why path can't be renamed, deleted or replaced while openScene (invalid if it has no file) is open, or empty if
		// it can: openScene is path, or is inside it
		std::string GetWhyInUse(const Filepath& path, const Filepath& openScene);

		// Moves a file or folder into the trash, with the thumbnails of the scenes and meshes in it. Not IsInTrash if it
		// couldn't, and nothing changes then.
		TrashedItem MoveToTrash(const Filepath& path);
		// Puts item, and its thumbnails, back where they were. False if something else has its name now, or it
		// couldn't be moved; it stays in the trash then.
		bool RestoreFromTrash(const TrashedItem& item);
		// Sends item on to the Recycle Bin and deletes its thumbnails, for when it can't be restored any more. It's moved
		// back where it was first, so the Recycle Bin records that folder, unless something else has its name now or
		// the folder has gone; it's sent from the trash then. False if it couldn't, and it's left in the trash for
		// EmptyTrash to try again (or, should that move fail, where it was, as an undo would leave it).
		bool Recycle(const TrashedItem& item);
		bool Recycle(const TrashedItem& item, SendToRecycleBin sendToRecycleBin);
		// Sends everything in the trash to the Recycle Bin, as Recycle does, for when nothing can restore it: when the
		// editor starts, in case it didn't close cleanly, and when it closes. Each entry of the trash records where its
		// item was; one that doesn't is sent from the trash.
		void EmptyTrash();
		void EmptyTrash(SendToRecycleBin sendToRecycleBin);

		// Where the trash is, which the Content Browser doesn't show
		const Filepath& GetTrashFolder();
	}
}
