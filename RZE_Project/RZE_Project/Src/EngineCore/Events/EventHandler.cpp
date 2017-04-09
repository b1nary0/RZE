#include "StdAfx.h"

#include "EventHandler.h"

EventHandler::EventHandler()
{
}

void EventHandler::RegisterEventImmediate(const Event& event)
{
	std::vector<EventHandlingInfo> notifyList = mEventNotifyMap[event.mInfo.mEventType];
	for (EventHandlingInfo& info : notifyList)
	{
		info.mCallback(event);
	}
}

void EventHandler::RegisterWindowEvent(const WindowEvent& windowEvent, const bool bFireImmediate)
{
	Event event;
	event.mWindowEvent = windowEvent;

	if (bFireImmediate)
	{
		RegisterEventImmediate(event);
	}
	{
		mEventQueue.push(event);
	}
}

void EventHandler::RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback)
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
