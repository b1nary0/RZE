#pragma once

#include <string>

#include <Utils/PrimitiveDefs.h>

#include <EngineCore/Input/KeyCodes.h>

#define MAX_KEYCODES_SUPPORTED 256
// What one notch of the mouse wheel adds to MouseState's wheel value (Windows' WHEEL_DELTA)
constexpr Int32 k_mouseWheelNotchDelta = 120;
#define MAX_MOUSE_BTN_SUPPORTED 5

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
		// The side buttons (mouse 4 and 5), which usually go back and forward
		MouseButton_Back,
		MouseButton_Forward,
		// Not a button: a queued move or wheel event
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
