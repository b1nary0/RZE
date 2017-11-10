#pragma once

#include <Utils/Functor.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;

class InputHandler
{
	struct KeyAction
	{
		U8 mKey;
		U16 mActionType;
	};

public:
	InputHandler();

	void RegisterEvents(EventHandler& eventHandler);
	void RegisterForEvent(U16 action, Functor<void, U8> callback);

	void ProcessEvents();

	void ProcessOSKeyboardEvent(EKeyEventType::T eventType, const U8 key);
	void ProcessOSMouseEvent(EMouseEventType::T eventType, const Vector2D& pos);

private:
	std::queue<KeyAction> mKeyPresses;
	std::map<U16, std::vector<Functor<void, U8>>> mNotifyMap;
};
