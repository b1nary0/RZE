#pragma once

#include <string>

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
};
