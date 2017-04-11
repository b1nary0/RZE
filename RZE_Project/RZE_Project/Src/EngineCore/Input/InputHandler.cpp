#include "StdAfx.h"

#include "InputHandler.h"
#include "EngineCore/Events/EventHandler.h"

InputHandler::InputHandler()
{
}

void InputHandler::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> keyInputCallback([this](const Event& event)
	{
		assert(event.mInfo.mEventType == EEventType::Key);
		if (event.mKeyEvent.mEventInfo.mEventSubType == EKeyEventType::Key_Pressed)
		{
			KeyAction keyAction;
			keyAction.mActionType = 0;
			keyAction.mKey = event.mKeyEvent.mKey;

			mKeyPresses.push(keyAction);
		}
	});

	eventHandler.RegisterForEvent(EEventType::Key, keyInputCallback);
}

void InputHandler::RegisterForEvent(UInt16 action, Functor<void, UInt8> callback)
{
	mNotifyMap[action].push_back(callback);
}

void InputHandler::ProcessEvents()
{
	while (!mKeyPresses.empty())
	{
		KeyAction action = mKeyPresses.front();
		mKeyPresses.pop();

		std::vector<Functor<void, UInt8>> notifyEntries = mNotifyMap[action.mActionType];
		for (auto& entry : notifyEntries)
		{
			entry(action.mKey);
		}
	}
}
