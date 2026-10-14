#pragma once

#include <Utils/Platform/Filepath.h>

#include <filesystem>
#include <fstream>
#include <string>

// A scratch folder next to the test executable (Filepaths are relative to it), empty when the test starts and removed
// again when it ends
class ScopedTestFolder
{
public:
	explicit ScopedTestFolder(const std::string& name)
		: m_folder(name)
	{
		std::filesystem::remove_all(m_folder.GetAbsolutePath());
		std::filesystem::create_directories(m_folder.GetAbsolutePath());
	}

	~ScopedTestFolder()
	{
		std::error_code ignored;
		std::filesystem::remove_all(m_folder.GetAbsolutePath(), ignored);
	}

	ScopedTestFolder(const ScopedTestFolder&) = delete;
	ScopedTestFolder& operator=(const ScopedTestFolder&) = delete;

public:
	const Filepath& Get() const { return m_folder; }

	// A path in the folder, which may not exist yet
	Filepath GetFile(const std::string& relativePath) const
	{
		return Filepath(m_folder.GetRelativePath() + "/" + relativePath);
	}

	// Writes a small file at relativePath, making its folders
	Filepath AddFile(const std::string& relativePath) const
	{
		const Filepath file = GetFile(relativePath);
		std::filesystem::create_directories(std::filesystem::path(file.GetAbsolutePath()).parent_path());
		std::ofstream(file.GetAbsolutePath()) << "test";
		return file;
	}

	void AddFolder(const std::string& relativePath) const
	{
		std::filesystem::create_directories(GetFile(relativePath).GetAbsolutePath());
	}

private:
	Filepath m_folder;
};
