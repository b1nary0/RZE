#include "StdAfx.h"

#include "Events.h"

WindowEvent::WindowEvent(const U16 eventSubType)
	: mEventInfo(EEventType::Window, eventSubType)
{
}

EventInfo::EventInfo(const U16 eventType, const U16 eventSubType)
	: mEventType(eventType)
	, mEventSubType(eventSubType)
{
}

KeyEvent::KeyEvent(const U16 eventSubType, const U8 key)
	: mEventInfo(EEventType::Key, eventSubType)
	, mKey(key)
{
}

MouseEvent::MouseEvent(const U16 eventSubType, const U32 posX, const U32 posY)
    : mEventInfo(EEventType::Mouse, eventSubType)
    , mPosX(posX)
    , mPosY(posY)
{

}
