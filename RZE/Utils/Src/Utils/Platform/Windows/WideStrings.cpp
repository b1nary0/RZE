#include <StdAfx.h>

#include <Utils/Platform/Windows/WideStrings.h>

#include <Windows.h>

namespace Conversions
{
	std::wstring StringToWString(const std::string& string)
	{
		// @NOTE passing 0 as last param returns size needed for conversion from mbstr to wstr
		// The size includes the null terminator, which the wstring keeps itself
		const int wstringSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, nullptr, 0);
		if (wstringSize <= 1)
		{
			return std::wstring();
		}

		std::wstring wstr(static_cast<size_t>(wstringSize), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wstr.data(), wstringSize);
		wstr.resize(static_cast<size_t>(wstringSize) - 1);
		return wstr;
	}
}
