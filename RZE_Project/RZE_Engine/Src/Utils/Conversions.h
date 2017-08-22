#pragma once

#include <string>

namespace Conversions
{
	std::wstring		StringToWString(const std::string& string);

    int					IntFromString(const std::string& string);
	std::string			StringFromInt(int value);

    std::string CreateResourceKeyFromPath(const std::string& string);
}