#pragma once

#include <Utils/Platform/Platform.h>

#include <algorithm>
#include <string>

// A relative path the way Filepath gives it on this platform, written with '/': NativePath("Assets/Scenes") is
// "Assets\Scenes" on Windows and stays "Assets/Scenes" elsewhere
inline std::string NativePath(std::string path)
{
	std::replace(path.begin(), path.end(), '/', Platform::k_pathSeparator);
	return path;
}
