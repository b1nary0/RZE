#pragma once

#include "Utils/Functor.h"

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
	
private:

	std::queue<KeyAction> mKeyPresses;
	std::map<U16, std::vector<Functor<void, U8>>> mNotifyMap;
};
