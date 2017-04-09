#pragma once

#include "Events.h"

#include <queue>
#include <map>

#include "Events.h"
#include "EngineCore/Utils/Functor.h"

class EventHandler
{

private:

	struct EventHandlingInfo
	{
		Event mEvent;
		Functor<void, const Event&> mCallback;
	};

public:

	EventHandler();

	void CreateWindowEvent();

	void RegisterEventImmediate(const Event& event);
	void RegisterWindowEvent(const WindowEvent& windowEvent, const bool bFireImmediate);

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

private:

	std::queue<Event> mEventQueue;
	std::map<UInt16, std::vector<EventHandlingInfo>> mEventNotifyMap;
};
