#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct StringUtils
{
	static std::string ExtractFileNameFromPath(const std::string filePath)
	{
		const char* const delim = "/\\";

		const size_t index = filePath.find_last_of(delim);

		if (index == std::string::npos)
		{
			return filePath;
		}

		const std::string fileName = filePath.substr(index + 1);
		return fileName;
	}

	// ASCII only: other characters are left as they are
	static std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	// ASCII only: other characters are left as they are
	static std::string ToUpper(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return text;
	}

	static bool EqualsIgnoringCase(const std::string& a, const std::string& b)
	{
		return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
			[](unsigned char x, unsigned char y) { return std::tolower(x) == std::tolower(y); });
	}

	static bool StartsWithIgnoringCase(const std::string& text, const std::string& prefix)
	{
		return text.size() >= prefix.size() && EqualsIgnoringCase(text.substr(0, prefix.size()), prefix);
	}

	static bool EndsWithIgnoringCase(const std::string& text, const std::string& suffix)
	{
		return text.size() >= suffix.size() && EqualsIgnoringCase(text.substr(text.size() - suffix.size()), suffix);
	}

	// Without the spaces and tabs at either end
	static std::string Trim(const std::string& text)
	{
		const size_t first = text.find_first_not_of(" \t");
		const size_t last = text.find_last_not_of(" \t");
		return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
	}

	template<class... Args>
	static std::string FormatString(const std::string& stringToFormat, Args... args)
	{
		/*
		 * #TODO(Josh)
		 * This function is ok for now but looks likely to be pretty expensive... Optimize later.
		 * Also, using magic numbers will have to find better way later.
		 */
		std::vector<char> buf(1024);
		sprintf_s(&buf[0], buf.size(), stringToFormat.c_str(), std::forward<Args>(args)...);

		return std::string(buf.begin(), buf.end());
	}
};
