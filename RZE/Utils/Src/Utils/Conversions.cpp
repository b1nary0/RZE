#include <Utils/Conversions.h>
#include <Utils/Platform/Filepath.h>
#include <Utils/Platform/Platform.h>

#include <string>

namespace Conversions
{
	int IntFromString(const std::string& string)
	{
		return atoi(string.c_str());
	}

	std::string StringFromInt(int value)
	{
		return std::to_string(value);
	}

	std::string StripAssetNameFromFilePath(const Filepath& filePath)
	{
		const std::string& assetPath = filePath.GetRelativePath();

		size_t pos = assetPath.find_last_of(Platform::k_pathSeparator);
		std::string assetName = assetPath.substr(pos + 1, assetPath.size());
		assetName = assetName.substr(0, assetName.find_last_of('.'));

		return assetName;
	}

}