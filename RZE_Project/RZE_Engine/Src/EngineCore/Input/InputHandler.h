#pragma once

#include "Utils/Functor.h"

class EventHandler;

class InputHandler
{
	struct KeyAction
	{
		UInt8 mKey;
		UInt16 mActionType;
	};

public:
	
	InputHandler();

	void RegisterEvents(EventHandler& eventHandler);

	void RegisterForEvent(UInt16 action, Functor<void, UInt8> callback);

	void ProcessEvents();
	
private:

	std::queue<KeyAction> mKeyPresses;
	std::map<UInt16, std::vector<Functor<void, UInt8>>> mNotifyMap;
};
