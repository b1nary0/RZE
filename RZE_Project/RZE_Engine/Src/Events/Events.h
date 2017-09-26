#pragma once

#include "EventTypes.h"

struct EventInfo
{
	friend struct Event;
	friend struct WindowEvent;
	friend struct KeyEvent;
	friend struct MouseEvent;

	EventInfo(const U16 eventType, const U16 eventSubType);

	U16 mEventType;
	U16 mEventSubType;

private:

	EventInfo() = default;
};

struct WindowEvent
{
	friend struct Event;

	WindowEvent() = default;
	WindowEvent(const U16 eventSubType);

	EventInfo mEventInfo;
};

struct KeyEvent
{
	friend struct Event;

	EventInfo mEventInfo;

	KeyEvent() = default;
	KeyEvent(const U16 eventSubType, const U8 key);

	U8 mKey;
};

struct MouseEvent
{
	friend struct Event;

	EventInfo mEventInfo;

	MouseEvent() = default;
	MouseEvent(const U16 eventSubType, const U32 posX, const U32 posY);

	U32 mPosX;
	U32 mPosY;
};

struct Event
{
	union
	{
		EventInfo mInfo;
		WindowEvent mWindowEvent;
		KeyEvent mKeyEvent;
		MouseEvent mMouseEvent;
	};
};

