#pragma once

#include <string>
#include <Utils/PrimitiveDefs.h>

class InputKey
{
public:
	InputKey(const std::string& displayName, Int32 keyCode);

private:
	InputKey();

private:
	std::string mDisplayName;
	Int32 mKeyCode;
};
