#include <StdAfx.h>

#include <Utils/Platform/Folders.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <filesystem>

namespace Folders
{
	std::vector<Filepath> ListSubfolders(const Filepath& folder)
	{
		std::vector<std::string> names;

		std::error_code error;
		std::filesystem::directory_iterator iterator(folder.GetAbsolutePath(), error);
		if (error)
		{
			return {};
		}

		for (const std::filesystem::directory_entry& item : iterator)
		{
			if (item.is_directory(error))
			{
				names.push_back(item.path().filename().string());
			}
		}

		std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b)
			{
				return StringUtils::ToLower(a) < StringUtils::ToLower(b);
			});

		std::vector<Filepath> subfolders;
		for (const std::string& name : names)
		{
			subfolders.emplace_back(folder.GetRelativePath() + "/" + name);
		}
		return subfolders;
	}

	std::vector<Filepath> FindFiles(const Filepath& folder, const std::string& extension)
	{
		std::vector<std::string> relativePaths;

		std::error_code error;
		const std::filesystem::path root(folder.GetAbsolutePath());
		std::filesystem::recursive_directory_iterator iterator(root, error);
		if (error)
		{
			return {};
		}

		for (const std::filesystem::directory_entry& item : iterator)
		{
			if (item.is_regular_file(error) && StringUtils::EqualsIgnoringCase(item.path().extension().string(), extension))
			{
				relativePaths.push_back(std::filesystem::relative(item.path(), root, error).generic_string());
			}
		}

		std::sort(relativePaths.begin(), relativePaths.end(), [](const std::string& a, const std::string& b)
			{
				return StringUtils::ToLower(a) < StringUtils::ToLower(b);
			});

		std::vector<Filepath> files;
		for (const std::string& relativePath : relativePaths)
		{
			files.emplace_back(folder.GetRelativePath() + "/" + relativePath);
		}
		return files;
	}
}
