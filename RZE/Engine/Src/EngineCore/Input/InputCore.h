#pragma once

#include <string>

#include <Utils/PrimitiveDefs.h>

#include <Windowing/WinKeyCodes.h>

#define MAX_KEYCODES_SUPPORTED 256
#define MAX_MOUSE_BTN_SUPPORTED 3

namespace EButtonState
{
	enum T : Int32
	{
		ButtonState_Pressed,
		ButtonState_Released,
		ButtonState_Hold
	};
}

namespace EMouseButton
{
	enum T : Int32
	{
		MouseButton_Left,
		MouseButton_Right,
		MouseButton_Middle,
		MouseButton_Move
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
	InputKey()
		: mDisplayName(' ')
		, mKeyCode(-1) {}

	InputKey(U8 displayName, Int32 keyCode);

	Int32 GetKeyCode() const { return mKeyCode; }

	inline bool IsValid() const { return mKeyCode >= 0; }

private:
	U8 mDisplayName;
	Int32 mKeyCode;
};
