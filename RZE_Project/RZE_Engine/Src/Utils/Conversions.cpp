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

}