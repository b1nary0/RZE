#pragma once

class Filepath;

namespace Conversions
{
	std::wstring		StringToWString(const std::string& string);

	int					IntFromString(const std::string& string);
	std::string			StringFromInt(int value);

	std::string CreateResourceKeyFromPath(const std::string& string);

	std::string StripAssetNameFromFilePath(const Filepath& filePath);
}