#include "StdAfx.h"

#include "Conversions.h"

namespace Conversions
{
	std::wstring StringToWString(const std::string& string)
	{
		// see: http://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(string);
	}

    int IntFromString(const std::string& string)
    {
        return atoi(string.c_str());
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

}