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

	// The extensions in a file-prompt style filter, for ListFolder: "*.scene;*.meshasset" gives { ".scene", ".meshasset" }.
	// A filter that allows any file ("*.*", or empty) gives none, which ListFolder also takes as any file.
	std::vector<std::string> ParseExtensionFilter(const std::string& filter);

	// How long ago time was, for a person: "Just now", "5 minutes ago", "3 days ago", or the date once it's months old
	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now);
}
