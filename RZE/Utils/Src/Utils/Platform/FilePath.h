#pragma once

#include <string>

//
// @implementation: will do this later, leaving as idea.
//

enum class EDirectoryContext
{
	Runtime,
	Tools
};

class Filepath
{
public:
	Filepath() = default;
	explicit Filepath(const std::string& path);
	~Filepath();

	static void CreateDir(const std::string& directoryPath);
	static Filepath FromAbsolutePathStr(const std::string& absolutePath);

	bool Exists();

	const std::string& GetAbsolutePath() const;
	const std::string& GetRelativePath() const;

	const std::string GetAbsoluteDirectoryPath() const;
	const std::string GetRelativeDirectoryPath() const;

	bool IsValid() const;

	static void SetDirectoryContext(EDirectoryContext context);

private:
	std::string mRelativePath;
	std::string mAbsolutePath;
};