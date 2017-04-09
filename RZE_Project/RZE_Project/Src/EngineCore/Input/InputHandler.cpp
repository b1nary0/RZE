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
			printf("InputHandler detected key press of %c\n", static_cast<char>(event.mKeyEvent.mKey));
		}
	});

	eventHandler.RegisterForEvent(EEventType::Key, keyInputCallback);
}
