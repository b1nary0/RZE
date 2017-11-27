#pragma once

#include <Utils/PrimitiveDefs.h>

namespace EEventType
{
	enum T : U16
	{
		Window,
		Key,
		Mouse
	};
}

namespace EWindowEventType
{
	enum T : U16
	{
		Window_Create,
		Window_Destroy,
		Window_Resize
	};
}

namespace EKeyEventType
{
	enum T : U16
	{
		Key_Pressed,
		Key_Released
	};
}

namespace EMouseEventType
{
	enum T : U16
	{
		Mouse_Move,
		Mouse_LClick,
		Mouse_LRelease
	};
}
