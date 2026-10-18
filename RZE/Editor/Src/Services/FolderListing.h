#pragma once

#include <Utils/Platform/Filepath.h>

#include <chrono>
#include <string>
#include <vector>

namespace Editor
{
	// One file or subfolder in a folder, as ListFolder finds it
	struct FolderEntry
	{
		// With its extension, e.g. "Sponza.scene"
		std::string Name;
		Filepath Path;
		bool IsFolder = false;
		std::chrono::system_clock::time_point ModifiedTime;
	};

	// The subfolders of folder, then its files with one of extensions (e.g. ".scene"; any file if empty), each group
	// sorted by name. Extensions and searchText match ignoring case; searchText keeps the entries whose name contains
	// it. Empty if the folder doesn't exist.
	std::vector<FolderEntry> ListFolder(const Filepath& folder, const std::vector<std::string>& extensions, const std::string& searchText = "");

	// The folders directly inside folder, sorted by name ignoring case. Empty if the folder doesn't exist.
	std::vector<Filepath> ListSubfolders(const Filepath& folder);

	// Every file under folder, in its subfolders too, with extension (e.g. ".scene", ignoring case), sorted by path.
	// Empty if the folder doesn't exist.
	std::vector<Filepath> FindFiles(const Filepath& folder, const std::string& extension);

	// The extensions in a file-prompt style filter, for ListFolder: "*.scene;*.meshasset" gives { ".scene", ".meshasset" }.
	// A filter that allows any file ("*.*", or empty) gives none, which ListFolder also takes as any file.
	std::vector<std::string> ParseExtensionFilter(const std::string& filter);

	// Why Windows can't use name for a file or folder (e.g. it's empty, has a character like ':', is a device name like
	// "CON", or ends with a dot), or empty if it can
	std::string GetFileNameError(const std::string& name);

	// A path for a typed name: Path is invalid and Error says why if the name can't be used
	struct ResolvedName
	{
		Filepath Path;
		std::string Error;
	};

	// Where a file name typed into a save prompt saves to: a file in folder, with extension added unless the name
	// already ends with it (ignoring case), e.g. "Level 2" -> "<folder>/Level 2.scene". The name is trimmed of spaces.
	ResolvedName ResolveSaveName(const Filepath& folder, const std::string& typedName, const std::string& extension);

	// What renaming path to a typed name gives: a folder takes the name as it is, and a file keeps its extension, added
	// unless the name already ends with it. The name is trimmed of spaces. An error too if something else already has
	// the name; only changing case, or keeping the name, is fine.
	ResolvedName ResolveRename(const Filepath& path, bool isFolder, const std::string& typedName);

	// A path in folder for a new file or folder called name that nothing has yet: name itself, or with " 2", " 3", ...
	// before its extension, e.g. "New Folder 2"
	Filepath MakeUniqueFileName(const Filepath& folder, const std::string& name);

	// How long ago time was, for a person: "Just now", "5 minutes ago", "3 days ago", or the date once it's months old
	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now);
}
