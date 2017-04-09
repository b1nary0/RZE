#include "StdAfx.h"

#include "Events.h"

WindowEvent::WindowEvent(const UInt16 eventSubType)
	: mEventInfo(EEventType::Window, eventSubType)
{
}

EventInfo::EventInfo(const UInt16 eventType, const UInt16 eventSubType)
	: mEventType(eventType)
	, mEventSubType(eventSubType)
{
}

KeyEvent::KeyEvent(const UInt16 eventSubType, const UInt8 key)
	: mEventInfo(EEventType::Key, eventSubType)
	, mKey(key)
{
}
