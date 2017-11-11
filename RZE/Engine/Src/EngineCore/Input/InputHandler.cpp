#include "StdAfx.h"

#include "InputHandler.h"
#include "Events/EventHandler.h"

InputHandler::InputHandler()
{
}

void InputHandler::RegisterEvents(EventHandler& eventHandler)
{
}

void InputHandler::RegisterForEvent(U16 action, Functor<void, U8> callback)
{
	//mNotifyMap[action].push_back(callback);
}

void InputHandler::ProcessEvents()
{
	while (!mKeyPresses.empty())
	{
		KeyAction action = mKeyPresses.front();
		mKeyPresses.pop();

		std::vector<Functor<void, Int32>> notifyEntries = mNotifyMap[action.bIsDown];
		for (auto& entry : notifyEntries)
		{
			entry(action.Key);
		}
	}
}

void InputHandler::OnKeyDown(const Int32 key, bool bIsRepeat)
{
	/* 
	 * This function should take the "raw" input and translate it into
	 * the first step of the abstraction process. Possibly immediately into a {Key}
	 * that we can then store in some state.
	 * Possibly send this info off to another processing stage? Should this class be
	 * an intermediate?
	 */


	// Temporary code until work out a better protocol for this... 
	// This is more basically stubbing than anything.
	KeyAction keyAction;
	keyAction.Key = key;
	keyAction.bIsDown = true;
	keyAction.bIsRepeat = bIsRepeat;

	mKeyPresses.push(keyAction);
}
