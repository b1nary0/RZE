#pragma once

#include "EventTypes.h"

struct EventInfo
{
	friend struct Event;
	friend struct WindowEvent;
	friend struct KeyEvent;

	EventInfo(const UInt16 eventType, const UInt16 eventSubType);

	UInt16 mEventType;
	UInt16 mEventSubType;

private:

	EventInfo() = default;
};

struct WindowEvent
{
	friend struct Event;

	WindowEvent() = default;
	WindowEvent(const UInt16 eventSubType);

	EventInfo mEventInfo;
};

struct KeyEvent
{
	friend struct Event;

	EventInfo mEventInfo;

	KeyEvent() = default;
	KeyEvent(const UInt16 eventSubType, const UInt8 key);

	UInt8 mKey;
};

struct Event
{
	union
	{
		EventInfo mInfo;
		WindowEvent mWindowEvent;
		KeyEvent mKeyEvent;
	};
};

