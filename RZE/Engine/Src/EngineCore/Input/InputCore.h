#pragma once

#include <string>
#include <Utils/PrimitiveDefs.h>

#define MAX_KEYCODES_SUPPORTED 256

class InputKey
{
public:
	InputKey(const char displayName, Int32 keyCode);

private:
	InputKey();

private:
	char mDisplayName;
	Int32 mKeyCode;
};
