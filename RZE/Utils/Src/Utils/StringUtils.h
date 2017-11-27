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
