#include <StdAfx.h>

#include <Utils/Conversions.h>
#include <Utils/Platform/Filepath.h>

#include <Windows.h>

#include <string>

namespace Conversions
{
	std::wstring StringToWString(const std::string& string)
	{
		// @NOTE passing 0 as last param returns size needed for conversion from mbstr to wstr
		const int wstringSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, nullptr, 0);
		std::wstring wstr;
		wstr.reserve(wstringSize);

		MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, &wstr[0], wstringSize);
		return wstr;
	}

	int IntFromString(const std::string& string)
	{
		return atoi(string.c_str());
	}

	std::string StringFromInt(int value)
	{
		return std::to_string(value);
	}

	std::string CreateResourceKeyFromPath(const std::string& string)
	{
		// #TODO this is a naive approach, will have to find a better solution later.
		std::string resourceKey = string;

		resourceKey = resourceKey.substr(0, resourceKey.find_last_of('.'));
		std::replace(resourceKey.begin(), resourceKey.end(), '/', ' ');
		resourceKey.erase(std::remove(resourceKey.begin(), resourceKey.end(), '.'), resourceKey.end());

		char c = resourceKey.front();
		while (c == ' ')
		{
			resourceKey.erase(resourceKey.begin());
			c = resourceKey.front();
		}

		std::replace(resourceKey.begin(), resourceKey.end(), ' ', '.');

		return resourceKey;
	}

	std::string StripAssetNameFromFilePath(const Filepath& filePath)
	{
		const std::string& assetPath = filePath.GetRelativePath();

		size_t pos = assetPath.find_last_of('\\');
		std::string assetName = assetPath.substr(pos + 1, assetPath.size());
		assetName = assetName.substr(0, assetName.find_last_of('.'));

		return assetName;
	}

}