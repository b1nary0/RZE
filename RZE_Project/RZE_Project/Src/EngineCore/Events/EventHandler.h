#pragma once

#include "EventTypes.h"

#include <queue>

class EventHandler
{
public:

	EventHandler() {}

private:

	std::queue<Event> mEventQueue;

};
