#pragma once

#include <string>
#include <Utils/PrimitiveDefs.h>

#define MAX_KEYCODES_SUPPORTED 256
#define MAX_MOUSE_BTN_SUPPORTED 3

namespace EButtonState
{
	enum T
	{
		ButtonState_Pressed,
		ButtonState_Released,
		ButtonState_Hold
	};
}

class InputKey
{
public:
	InputKey(const char displayName, Int32 keyCode);

	Int32 GetKeyCode() const { return mKeyCode; }

private:
	InputKey();

private:
	char mDisplayName;
	Int32 mKeyCode;
};
