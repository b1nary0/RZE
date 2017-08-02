#include "StdAfx.h"

#include "EventHandler.h"

EventHandler::EventHandler()
{
}

void EventHandler::PostEvent(const Event& event, const bool bFireImmediate)
{
	if (bFireImmediate)
	{
		PostEventImmediate(event);
	}
	else
	{
		mEventQueue.push(event);
	}
}

void EventHandler::PostEventImmediate(const Event& event)
{
	std::vector<EventHandlingInfo> notifyList = mEventNotifyMap[event.mInfo.mEventType];
	for (EventHandlingInfo& info : notifyList)
	{
		info.mCallback(event);
	}
}

void EventHandler::PostWindowEvent(const WindowEvent& windowEvent, const bool bFireImmediate /*= false*/)
{
	Event event;
	event.mWindowEvent = windowEvent;
	PostEvent(event, bFireImmediate);
}

void EventHandler::PostKeyEvent(const KeyEvent& keyEvent, const bool bFireImmediate /*= false*/)
{
	Event event;
	event.mKeyEvent = keyEvent;
	PostEvent(event, bFireImmediate);
}

void EventHandler::PostMouseEvent(const MouseEvent& mouseEvent, const bool bFireImmediate /*= false*/)
{
    Event event;
    event.mMouseEvent = mouseEvent;
    PostEvent(event, bFireImmediate);
}

void EventHandler::RegisterForEvent(const U16 eventType, Functor<void, const Event&> callback)
{
	Event event;
	event.mInfo.mEventType = eventType;

	EventHandlingInfo info;
	info.mEvent = event;
	info.mCallback = callback;

	mEventNotifyMap[eventType].push_back(info);
}

void EventHandler::ProcessEvents()
{
	while (!mEventQueue.empty())
	{
		Event event = mEventQueue.front();
		mEventQueue.pop();

		std::vector<EventHandlingInfo> eventEntries = mEventNotifyMap[event.mInfo.mEventType];
		for (auto& entry : eventEntries)
		{
			entry.mCallback(event);
		}
	}
}
