#include <TestFolder.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

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

void ForEachTruncation(const Filepath& file, const Filepath& truncatedFile, const std::function<void(size_t length)>& check)
{
	std::ifstream input(file.GetAbsolutePath(), std::ios::binary);
	const std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

	for (size_t length = 0; length < bytes.size(); ++length)
	{
		{
			std::ofstream output(truncatedFile.GetAbsolutePath(), std::ios::binary | std::ios::trunc);
			output.write(bytes.data(), static_cast<std::streamsize>(length));
		}
		check(length);
	}
}
