#pragma once

#include "Utils/PrimitiveDefs.h"

namespace EEventType
{
	enum Type : U16
	{
		Window,
		Key,
        Mouse
	};
}

namespace EWindowEventType
{
	enum Type : U16
	{
		Window_Create,
		Window_Destroy
	};
}

namespace EKeyEventType
{
	enum Type : U16
	{
		Key_Pressed,
		Key_Released
	};
}

namespace EMouseEventType
{
    // #TODO(Josh) change the rest of these enums to use T instead of Type
    enum T : U16
    {
        Mouse_Move,
        Mouse_Click
    };
}
