#include "StdAfx.h"

#include "Events.h"

WindowEvent::WindowEvent(const UInt16 eventType, const UInt16 eventSubType)
	: mEventInfo(eventType, eventSubType)
{
}

EventInfo::EventInfo(const UInt16 eventType, const UInt16 eventSubType)
	: mEventType(eventType)
	, mEventSubType(eventSubType)
{
}
