#include <Services/FolderListing.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <cstring>
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

	// Without the spaces and tabs at either end
	std::string Trim(const std::string& text)
	{
		const size_t first = text.find_first_not_of(" \t");
		const size_t last = text.find_last_not_of(" \t");
		return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
	}

	// name with extension added, unless it already ends with it (ignoring case)
	std::string WithExtension(const std::string& name, const std::string& extension)
	{
		const bool hasExtension = name.size() > extension.size()
			&& StringUtils::EqualsIgnoringCase(name.substr(name.size() - extension.size()), extension);
		return hasExtension ? name : name + extension;
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

	std::vector<std::string> ParseExtensionFilter(const std::string& filter)
	{
		std::vector<std::string> extensions;

		size_t start = 0;
		while (start <= filter.size())
		{
			const size_t end = std::min(filter.find(';', start), filter.size());
			std::string pattern = filter.substr(start, end - start);
			start = end + 1;

			// "*.scene" -> ".scene"; "*.*" matches anything, so no list is needed
			if (!pattern.empty() && pattern[0] == '*')
			{
				pattern.erase(0, 1);
			}
			if (pattern == ".*" || pattern == "*")
			{
				return {};
			}
			if (!pattern.empty())
			{
				extensions.push_back(pattern);
			}
		}

		return extensions;
	}

	std::string GetFileNameError(const std::string& name)
	{
		if (name.empty())
		{
			return "Enter a name";
		}

		const char* const invalidCharacters = "\\/:*?\"<>|";
		const bool hasInvalidCharacter = std::any_of(name.begin(), name.end(),
			[invalidCharacters](unsigned char c) { return c < 32 || std::strchr(invalidCharacters, c) != nullptr; });
		if (hasInvalidCharacter)
		{
			return "A name can't contain \\ / : * ? \" < > |";
		}

		// Windows drops a trailing dot, so the file wouldn't be named what was typed
		if (name.back() == '.')
		{
			return "A name can't end with a dot";
		}

		// Device names, which Windows keeps for itself whatever extension follows, e.g. "CON.scene"
		static const char* const s_reserved[] = { "CON", "PRN", "AUX", "NUL",
			"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
			"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
		const std::string beforeDot = name.substr(0, name.find('.'));
		const std::string trimmed = beforeDot.substr(0, beforeDot.find_last_not_of(' ') + 1);
		const bool isReserved = std::any_of(std::begin(s_reserved), std::end(s_reserved),
			[&trimmed](const char* reserved) { return StringUtils::EqualsIgnoringCase(trimmed, reserved); });
		if (isReserved)
		{
			return "\"" + trimmed + "\" is a name Windows keeps for itself";
		}

		return std::string();
	}

	ResolvedName ResolveSaveName(const Filepath& folder, const std::string& typedName, const std::string& extension)
	{
		ResolvedName result;

		const std::string name = Trim(typedName);
		result.Error = GetFileNameError(name);
		if (!result.Error.empty())
		{
			return result;
		}

		result.Path = Filepath(folder.GetRelativePath() + "/" + WithExtension(name, extension));
		return result;
	}

	ResolvedName ResolveRename(const Filepath& path, bool isFolder, const std::string& typedName)
	{
		ResolvedName result;

		const std::string typed = Trim(typedName);
		result.Error = GetFileNameError(typed);
		if (!result.Error.empty())
		{
			return result;
		}

		const std::string name = isFolder ? typed : WithExtension(typed, path.GetExtension());
		const Filepath renamed(path.GetParentFolder().GetRelativePath() + "/" + name);
		// Windows ignores case, so a name that only changes case finds the file itself
		if (!StringUtils::EqualsIgnoringCase(name, path.GetFileName()) && renamed.Exists())
		{
			result.Error = "There's already something called \"" + name + "\" here";
			return result;
		}

		result.Path = renamed;
		return result;
	}

	Filepath MakeUniqueFileName(const Filepath& folder, const std::string& name)
	{
		const size_t dot = name.rfind('.');
		const bool hasExtension = dot != std::string::npos && dot > 0;
		const std::string stem = hasExtension ? name.substr(0, dot) : name;
		const std::string extension = hasExtension ? name.substr(dot) : std::string();

		Filepath path(folder.GetRelativePath() + "/" + name);
		for (int number = 2; path.Exists(); ++number)
		{
			path = Filepath(folder.GetRelativePath() + "/" + stem + " " + std::to_string(number) + extension);
		}
		return path;
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
