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

namespace EAxisType
{
	enum T
	{
		AxisType_Vector,
		AxisType_Float
	};
}

// #TODO(Josh) Don't like this but iterate over later
namespace EAxisBinding
{
	enum T
	{
		AxisBinding_Mouse
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
