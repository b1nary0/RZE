#pragma once

#include <string>

namespace Conversions
{
	std::wstring StringToWString(const std::string& string);

    int IntFromString(const std::string& string);
}