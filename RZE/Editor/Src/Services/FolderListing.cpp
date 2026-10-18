#include <Services/FolderListing.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <filesystem>

namespace
{
	// C++17 has no clock_cast, so this goes through each clock's now(); close enough to show to a person
	std::chrono::system_clock::time_point ToSystemTime(std::filesystem::file_time_type fileTime)
	{
		using namespace std::chrono;
		return time_point_cast<system_clock::duration>(fileTime - std::filesystem::file_time_type::clock::now() + system_clock::now());
	}
}

namespace Editor
{
	std::vector<FolderEntry> ListFolder(const Filepath& folder, const std::vector<std::string>& extensions, const std::string& searchText)
	{
		std::vector<FolderEntry> entries;

		std::error_code error;
		std::filesystem::directory_iterator iterator(folder.GetAbsolutePath(), error);
		if (error)
		{
			return entries;
		}

		std::vector<std::string> lowerExtensions;
		for (const std::string& extension : extensions)
		{
			lowerExtensions.push_back(StringUtils::ToLower(extension));
		}
		const std::string lowerSearch = StringUtils::ToLower(searchText);

		for (const std::filesystem::directory_entry& item : iterator)
		{
			const bool isFolder = item.is_directory(error);
			if (!isFolder && !item.is_regular_file(error))
			{
				continue;
			}

			const std::string name = item.path().filename().string();
			if (name.front() == '.')
			{
				continue;
			}
			if (!isFolder && !lowerExtensions.empty())
			{
				const std::string extension = StringUtils::ToLower(item.path().extension().string());
				if (std::find(lowerExtensions.begin(), lowerExtensions.end(), extension) == lowerExtensions.end())
				{
					continue;
				}
			}

			if (!lowerSearch.empty() && StringUtils::ToLower(name).find(lowerSearch) == std::string::npos)
			{
				continue;
			}

			FolderEntry entry;
			entry.Name = name;
			entry.Path = Filepath(folder.GetRelativePath() + "/" + name);
			entry.IsFolder = isFolder;
			entry.ModifiedTime = ToSystemTime(item.last_write_time(error));
			entries.push_back(std::move(entry));
		}

		std::sort(entries.begin(), entries.end(), [](const FolderEntry& a, const FolderEntry& b)
			{
				if (a.IsFolder != b.IsFolder)
				{
					return a.IsFolder;
				}
				return StringUtils::ToLower(a.Name) < StringUtils::ToLower(b.Name);
			});

		return entries;
	}
}
