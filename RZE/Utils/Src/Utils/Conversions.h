#pragma once

#include <string>

class Filepath;

// (StringToWString, which Windows APIs need, is in Utils/Platform/Windows/WideStrings.h)
namespace Conversions
{
	int					IntFromString(const std::string& string);
	std::string			StringFromInt(int value);

	std::string StripAssetNameFromFilePath(const Filepath& filePath);
}