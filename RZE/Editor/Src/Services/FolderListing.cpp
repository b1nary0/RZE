#include <Services/FolderListing.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <ctime>
#include <filesystem>

namespace
{
	// C++17 has no clock_cast, so this goes through each clock's now(); close enough to show to a person
	std::chrono::system_clock::time_point ToSystemTime(std::filesystem::file_time_type fileTime)
	{
		using namespace std::chrono;
		return time_point_cast<system_clock::duration>(fileTime - std::filesystem::file_time_type::clock::now() + system_clock::now());
	}

	std::string Plural(long long count, const char* unit)
	{
		return std::to_string(count) + " " + unit + (count == 1 ? "" : "s") + " ago";
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

	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now)
	{
		using namespace std::chrono;

		const long long seconds = duration_cast<std::chrono::seconds>(now - time).count();
		if (seconds < 60)
		{
			return "Just now";
		}

		const long long minutes = seconds / 60;
		if (minutes < 60)
		{
			return Plural(minutes, "minute");
		}

		const long long hours = minutes / 60;
		if (hours < 24)
		{
			return Plural(hours, "hour");
		}

		const long long days = hours / 24;
		if (days < 30)
		{
			return days == 1 ? std::string("Yesterday") : Plural(days, "day");
		}

		const std::time_t asTimeT = system_clock::to_time_t(time);
		std::tm local;
		localtime_s(&local, &asTimeT);
		char buffer[32];
		std::strftime(buffer, sizeof(buffer), "%b %d, %Y", &local);
		return buffer;
	}
}
