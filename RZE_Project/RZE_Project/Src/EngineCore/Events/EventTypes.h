#pragma once

#include "EngineCore/Utils/PrimitiveDefs.h"

namespace EEventType
{
	enum Type : UInt16
	{
		Window,
		Key
	};
}

namespace EWindowEventType
{
	enum Type : UInt16
	{
		Window_Destroy
	};
}

namespace EKeyEventType
{
	enum Type : UInt16
	{
		Key_Pressed,
		Key_Released
	};
}
