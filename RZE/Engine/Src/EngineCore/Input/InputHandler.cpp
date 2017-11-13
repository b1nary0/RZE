#include "StdAfx.h"

#include "InputHandler.h"
#include "Events/EventHandler.h"

InputHandler::InputHandler()
{
}

void InputHandler::Initialize()
{
	mInputKeyRegistry.reserve(MAX_KEYCODES_SUPPORTED);
	for (U32 keyCodeIdx = 0; keyCodeIdx < MAX_KEYCODES_SUPPORTED; keyCodeIdx++)
	{
		// #TODO(Josh) Possibly do a better cast here -- maybe throw in a function too? Do some research
		mInputKeyRegistry.emplace_back(static_cast<char>(keyCodeIdx), keyCodeIdx);
	}
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

	const InputKey& inputKey = mInputKeyRegistry[key];
}
