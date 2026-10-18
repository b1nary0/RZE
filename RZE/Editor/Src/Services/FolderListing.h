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
}
