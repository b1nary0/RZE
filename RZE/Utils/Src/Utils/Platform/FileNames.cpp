#include <StdAfx.h>

#include <Utils/Platform/FileNames.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <cstring>
#include <iterator>

namespace FileNames
{
	std::string GetError(const std::string& name)
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

	std::string WithExtension(const std::string& name, const std::string& extension)
	{
		// Longer than the extension, so a name that's only ".scene" still gets one
		const bool hasExtension = name.size() > extension.size() && StringUtils::EndsWithIgnoringCase(name, extension);
		return hasExtension ? name : name + extension;
	}

	Filepath MakeUnique(const Filepath& folder, const std::string& name)
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

	ResolvedName ResolveSave(const Filepath& folder, const std::string& typedName, const std::string& extension)
	{
		ResolvedName result;

		const std::string name = StringUtils::Trim(typedName);
		result.Error = GetError(name);
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

		const std::string typed = StringUtils::Trim(typedName);
		result.Error = GetError(typed);
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
}
