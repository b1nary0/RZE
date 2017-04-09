#pragma once

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

private:


};
