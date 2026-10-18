#pragma once

#include <Utils/Platform/Filepath.h>

#include <string>

namespace Editor
{
	// Changes to the project's files that keep the editor's own files beside them in step: the thumbnails rendered for
	// scenes and meshes (Thumbnails). Nothing that refers to a file by its path, like a scene using a mesh or a mesh
	// using a material, is updated.
	namespace AssetFiles
	{
		// Renames a file or folder to to, in the same folder (File::Move), and a scene's or mesh's thumbnail with it.
		// False if it couldn't, and nothing changes then.
		bool Rename(const Filepath& from, const Filepath& to);

		// Moves a file or folder to the Recycle Bin, and deletes the thumbnails of the scenes and meshes that went with
		// it. False if it couldn't, and nothing changes then.
		bool Delete(const Filepath& path);

		// Makes a folder in parent called name, or numbered if that's taken (FileNames::MakeUnique). Invalid if it couldn't.
		Filepath CreateFolder(const Filepath& parent, const std::string& name);
	}
}
