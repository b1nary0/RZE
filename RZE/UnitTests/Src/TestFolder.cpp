#include <TestFolder.h>

#include <filesystem>
#include <system_error>

void RemoveEmptyFolders(const Filepath& folder)
{
	// A Filepath is relative to the executable's folder, so the chain of parents ends before reaching it
	for (Filepath current = folder; current.IsValid(); current = current.GetParentFolder())
	{
		const std::filesystem::path path(current.GetAbsolutePath());
		std::error_code error;
		if (!std::filesystem::exists(path, error))
		{
			continue;
		}
		if (!std::filesystem::is_directory(path, error) || !std::filesystem::is_empty(path, error)
			|| !std::filesystem::remove(path, error))
		{
			return;
		}
	}
}
