#pragma once

#include "Utils/PrimitiveDefs.h"

namespace EEventType
{
	enum Type : U16
	{
		Window,
		Key
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
