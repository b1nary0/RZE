#pragma once

// UTF-8 to the UTF-16 that Win32 and D3D APIs take. Windows only, so only Windows code includes it.

#include <string>

namespace Conversions
{
	std::wstring StringToWString(const std::string& string);
}
