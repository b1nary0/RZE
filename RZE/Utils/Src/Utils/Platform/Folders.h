#pragma once

#include <Utils/Platform/Filepath.h>

#include <string>
#include <vector>

// What's in a folder on disk
namespace Folders
{
	// The folders directly inside folder, sorted by name ignoring case. Empty if the folder doesn't exist.
	std::vector<Filepath> ListSubfolders(const Filepath& folder);

	// Every file under folder, in its subfolders too, with extension (e.g. ".scene", ignoring case), sorted by path.
	// Empty if the folder doesn't exist.
	std::vector<Filepath> FindFiles(const Filepath& folder, const std::string& extension);
}
