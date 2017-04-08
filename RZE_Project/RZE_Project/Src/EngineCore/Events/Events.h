#pragma once

#include "EventTypes.h"

struct EventInfo
{

};

struct WindowEvent
{
	friend struct Event;
};

struct Event
{
	union
	{
		WindowEvent mWindowEvent;
	};
};

