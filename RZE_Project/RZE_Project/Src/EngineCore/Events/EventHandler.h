#pragma once

#include "Events.h"

#include <queue>
#include <functional>

class EventHandler
{
	typedef std::function<void(Event* const)> EventHandlerFunc;

private:

	struct EventHandlingInfo
	{
		EventHandlerFunc mHandlerFunc;
		Event mEvent;
	};

public:

	struct EventHandlingProtocol
	{
		EventHandlerFunc mHandlerFunc;
	};

	EventHandler();

	void CreateWindowEvent();

	EventHandler& Get();

private:

	std::queue<EventHandlingInfo> mEventQueue;

};
