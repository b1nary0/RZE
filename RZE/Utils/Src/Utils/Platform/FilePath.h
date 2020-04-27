#pragma once

#include <string>

//
// @implementation: will do this later, leaving as idea.
//

class FilePath
{
public:
	FilePath() = default;
	explicit FilePath(const std::string& path);
	// #TODO
	// Hacky constructor to read from non-runtime asset paths
	// until the FilePath infrastructure understands different directory
	// contexts.
	FilePath(const std::string& path, const bool isCustomPath);
	~FilePath();

	const std::string& GetAbsolutePath() const;
	const std::string& GetRelativePath() const;

	const std::string GetAbsoluteDirectoryPath() const;
	const std::string GetRelativeDirectoryPath() const;

	bool IsValid() const;

private:
	std::string mRelativePath;
	std::string mAbsolutePath;
};