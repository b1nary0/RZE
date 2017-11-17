#include <StdAfx.h>
#include <EngineCore/Input/InputHandler.h>

#include <DebugUtils/DebugServices.h>

#include <EngineCore/Platform/Timers/HiResTimer.h>

#include <Events/EventHandler.h>

InputHandler::InputHandler()
{
}

void InputHandler::Initialize()
{
	mKeyboardState.Reset();

	mInputKeyRegistry.reserve(MAX_KEYCODES_SUPPORTED);
	for (U32 keyCodeIdx = 0; keyCodeIdx < MAX_KEYCODES_SUPPORTED; keyCodeIdx++)
	{
		// #TODO(Josh) Possibly do a better cast here -- maybe throw in a function too? Do some research
		mInputKeyRegistry.emplace_back(static_cast<char>(keyCodeIdx), keyCodeIdx);
	}
}

void InputHandler::BindAction(Int32 keyCode, EButtonState::T buttonState, Functor<void, const InputKey&> func)
{
	KeyboardActionBinding actionBinding;
	actionBinding.ActionName = "Test";
	actionBinding.ButtonState = buttonState;
	actionBinding.Func = func;

	mKeyboardBindings[keyCode] = actionBinding;
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

	mKeyboardState.PrevKeyStates[key] = mKeyboardState.CurKeyStates[key];
	mKeyboardState.CurKeyStates[key] = true;

 	InputKey& inputKey = mInputKeyRegistry[key];
	RaiseKeyEvent(inputKey);
}

void InputHandler::OnKeyUp(const Int32 key)
{
	mKeyboardState.PrevKeyStates[key] = mKeyboardState.CurKeyStates[key];
	mKeyboardState.CurKeyStates[key] = false;

	const InputKey& inputKey = mInputKeyRegistry[key];
	RaiseKeyEvent(inputKey);
}

void InputHandler::RaiseKeyEvent(const InputKey& inputKey)
{
	auto& bindingIt = mKeyboardBindings.find(inputKey.GetKeyCode());
	if (bindingIt != mKeyboardBindings.end())
	{
	 	EButtonState::T buttonState = mKeyboardState.GetButtonState(inputKey.GetKeyCode());
	 	KeyboardActionBinding& actionBinding = (*bindingIt).second;
	 	if (actionBinding.ButtonState == buttonState)
	 	{
	 		actionBinding.Func(inputKey);
	 	}
	}
}