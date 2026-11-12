#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

// An exception to "no function bodies in headers" (CLAUDE.md): the one-expression helpers below stay here, where they
// can be inlined into the per-item loops that call them (filters, searches, file-name matching), and so does the
// template. Anything longer goes in StringUtils.cpp.
struct StringUtils
{
	// Copies source into the buffer, cut short if it doesn't fit, and always null-terminated (strncpy_s with _TRUNCATE)
	static void CopyTruncated(char* destination, size_t destinationSize, std::string_view source);

	template <size_t Size>
	static void CopyTruncated(char (&destination)[Size], std::string_view source)
	{
		CopyTruncated(destination, Size, source);
	}

	// After the last / or \\, or all of it if there's neither
	static std::string ExtractFileNameFromPath(const std::string& filePath);

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

	// Whether text contains part anywhere, ignoring case; true for an empty part
	static bool ContainsIgnoringCase(const std::string& text, const std::string& part)
	{
		return std::search(text.begin(), text.end(), part.begin(), part.end(),
			[](unsigned char x, unsigned char y) { return std::tolower(x) == std::tolower(y); }) != text.end();
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
	static std::string Trim(const std::string& text);

	// A count to 3 significant figures, with K, M, G or T past 999: "999", "1.00K", "12.3K", "362K", "13.8M". Below
	// 1000, to the nearest whole number. For counts shown where their width shouldn't change much, such as stats.
	static std::string FormatCount(double count);
};
