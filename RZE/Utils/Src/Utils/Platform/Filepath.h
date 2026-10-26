#pragma once

#include <string>

// Where relative paths like "Assets/..." resolve to
enum class EDirectoryContext
{
	// Beside the executable, for a packaged game
	Runtime,
	// The source tree: the folder above _build\ (or beside the executable when it isn't under _build\)
	Source
};

class Filepath
{
public:
	Filepath() = default;
	explicit Filepath(const std::string& path);
	~Filepath();

	static void CreateDir(const std::string& directoryPath);
	static Filepath FromAbsolutePathStr(const std::string& absolutePath);

	bool Exists() const;

	const std::string& GetAbsolutePath() const;
	const std::string& GetRelativePath() const;

	const std::string GetAbsoluteDirectoryPath() const;
	const std::string GetRelativeDirectoryPath() const;

	// The file or folder's own name, e.g. "Sponza.scene"
	std::string GetFileName() const;
	// GetFileName without its extension, e.g. "Sponza"
	std::string GetStem() const;
	// With its dot, e.g. ".scene"; empty if it has none
	std::string GetExtension() const;

	// The folder this is in, e.g. "Assets/Scenes" for "Assets/Scenes/Sponza.scene"; invalid if it isn't in one
	Filepath GetParentFolder() const;
	// Whether this is folder, or anywhere under it. Compares relative paths, ignoring case.
	bool IsInside(const Filepath& folder) const;

	bool IsValid() const;

	static void SetDirectoryContext(EDirectoryContext context);
	
private:
	std::string m_relativePath;
	std::string m_absolutePath;
};