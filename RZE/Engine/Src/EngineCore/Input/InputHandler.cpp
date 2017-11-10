#include "StdAfx.h"

#include "InputHandler.h"
#include "Events/EventHandler.h"

InputHandler::InputHandler()
{
}

void InputHandler::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> keyInputCallback([this](const Event& event)
	{
		AssertEqual(event.mInfo.mEventType, EEventType::Key);
		KeyAction keyAction;
		keyAction.mActionType = event.mKeyEvent.mEventInfo.mEventSubType;
		keyAction.mKey = event.mKeyEvent.mKey;

		mKeyPresses.push(keyAction);
	});

	eventHandler.RegisterForEvent(EEventType::Key, keyInputCallback);
}

void InputHandler::RegisterForEvent(U16 action, Functor<void, U8> callback)
{
	mNotifyMap[action].push_back(callback);
}

void InputHandler::ProcessEvents()
{
	while (!mKeyPresses.empty())
	{
		KeyAction action = mKeyPresses.front();
		mKeyPresses.pop();

		std::vector<Functor<void, U8>> notifyEntries = mNotifyMap[action.mActionType];
		for (auto& entry : notifyEntries)
		{
			entry(action.mKey);
		}
	}
}
