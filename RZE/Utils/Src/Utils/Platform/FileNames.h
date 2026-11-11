#pragma once

#include <Utils/Platform/Filepath.h>

#include <string>
#include <vector>

// The rules for naming files and folders, and what a name a person typed means as a path
namespace FileNames
{
	// Why Windows can't use name for a file or folder (e.g. it's empty, has a character like ':', is a device name like
	// "CON", or ends with a dot), or empty if it can
	std::string GetError(const std::string& name);

	// name with extension added unless it already ends with it (ignoring case), e.g. "Level 2" -> "Level 2.scene"
	std::string WithExtension(const std::string& name, const std::string& extension);

	// Whether something in path's folder already has path's name, ignoring case as Windows does, on every platform: a
	// project with two names that differ only in case couldn't be used on Windows. Except, if given, the file or folder
	// itself (e.g. the one being renamed), which only changing case doesn't make something else.
	bool IsTaken(const Filepath& path, const Filepath& itself = Filepath());

	// What's in path's folder with path's name ignoring case, as Windows would open it; invalid if nothing is. For files
	// named in data written on Windows, e.g. a texture an .mtl names "controls.tga" that's "Controls.tga" on disk.
	Filepath FindIgnoringCase(const Filepath& path);

	// A path in folder for a new file or folder called name that nothing has yet (IsTaken): name itself, or with " 2",
	// " 3", ... before its extension, e.g. "New Folder 2"
	Filepath MakeUnique(const Filepath& folder, const std::string& name);

	// The extensions in a file-prompt style filter, like a property's asset filter: "*.scene;*.meshasset" gives
	// { ".scene", ".meshasset" }. A filter that allows any file ("*.*", or empty) gives none.
	std::vector<std::string> ParseExtensionFilter(const std::string& filter);

	// Whether name (a file's name or path) ends with one of extensions (e.g. ".scene"), ignoring case. Any name does
	// if there are none, like ParseExtensionFilter gives for a filter that allows any file.
	bool MatchesExtensions(const std::string& name, const std::vector<std::string>& extensions);

	// A path for a typed name: Path is invalid and Error says why if the name can't be used
	struct ResolvedName
	{
		Filepath Path;
		std::string Error;
	};

	// Where a file name typed into a save prompt saves to: a file in folder, with extension added unless the name
	// already ends with it (ignoring case), e.g. "Level 2" -> "<folder>/Level 2.scene". The name is trimmed of spaces.
	ResolvedName ResolveSave(const Filepath& folder, const std::string& typedName, const std::string& extension);

	// What renaming path to a typed name gives: a folder takes the name as it is, and a file keeps its extension, added
	// unless the name already ends with it. The name is trimmed of spaces. An error too if something else already has
	// the name; only changing case, or keeping the name, is fine.
	ResolvedName ResolveRename(const Filepath& path, bool isFolder, const std::string& typedName);
}
