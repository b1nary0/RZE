#pragma once

#include <EngineCore/Input/InputCore.h>

#include <Utils/Functor.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;

class InputHandler
{
	struct KeyAction
	{
		Int32 Key;
		bool bIsDown;
		bool bIsRepeat;
	};

public:
	InputHandler();

	void Initialize();

	void RegisterEvents(EventHandler& eventHandler);
	void RegisterForEvent(U16 action, Functor<void, U8> callback);

	void ProcessEvents();

	void OnKeyDown(const Int32 key, bool bIsRepeat);

private:
	std::vector<InputKey> mInputKeyRegistry;

	std::queue<KeyAction> mKeyPresses;
	std::map<bool, std::vector<Functor<void, Int32>>> mNotifyMap;
};
