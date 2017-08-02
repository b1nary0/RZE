#pragma once

#include <queue>
#include <map>

#include "Events.h"

#include "Utils/Functor.h"

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

	void PostEventImmediate(const Event& event);

	void PostWindowEvent(const WindowEvent& windowEvent, const bool bFireImmediate = false);
	void PostKeyEvent(const KeyEvent& keyEvent, const bool bFireImmediate = false);
    void PostMouseEvent(const MouseEvent& mouseEvent, const bool bFireImmediate = false);

	void RegisterForEvent(const U16 eventType, Functor<void, const Event&> callback);

	void ProcessEvents();

private:

	void PostEvent(const Event& event, const bool bFireImmediate);

	std::queue<Event> mEventQueue;
	std::map<U16, std::vector<EventHandlingInfo>> mEventNotifyMap;
};
