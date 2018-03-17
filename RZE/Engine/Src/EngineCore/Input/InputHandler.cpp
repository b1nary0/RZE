#include <StdAfx.h>
#include <EngineCore/Input/InputHandler.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/Platform/Timers/HiResTimer.h>

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

void InputHandler::RaiseEvents()
{
	while (!mKeyActionQueue.empty())
	{
		KeyboardAction action = mKeyActionQueue.front();
		mKeyActionQueue.pop();

		AssertExpr(action.Key.IsValid());
		RaiseKeyEvent(action.Key);
	}

	while (!mMouseActionQueue.empty())
	{
		MouseAction action = mMouseActionQueue.front();
		mMouseActionQueue.pop();

		if (action.Button == EMouseButton::MouseButton_Move)
		{
			RaiseMouseAxisEvent(action.Position, action.Wheel);
		}
		else
		{
			RaiseMouseButtonEvent(action.Button, action.State, action.Position.X(), action.Position.Y());
		}
	}
}

void InputHandler::BindAction(Int32 keyCode, EButtonState::T buttonState, KeyActionFunc func)
{
	KeyboardActionBinding actionBinding;
	actionBinding.ActionName = "Test";
	actionBinding.Func = func;

	actionBinding.Action.Key = mInputKeyRegistry[keyCode];
	actionBinding.Action.State = buttonState;

	mKeyboardBindings[keyCode] = actionBinding;
}

void InputHandler::BindAxis(EAxisBinding::T bindingType, EAxisType::T axisType, Functor<void, const Vector3D&, Int32> func)
{
	std::vector<AxisBinding>& bindings = mAxisBindings[bindingType];
	bindings.emplace_back();

	AxisBinding& binding = bindings.back();
	binding.AxisType = axisType;
	binding.BindingType = bindingType;
	binding.Func = func;
}

void InputHandler::BindMouseAction(EMouseButton::T button, EButtonState::T state, MouseActionFunc func)
{
	mMouseBtnBindings[button].emplace_back(button, state, func);
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
	KeyboardAction action;
	action.Key = inputKey;
	action.State = mKeyboardState.GetButtonState(key);

	mKeyActionQueue.push(std::move(action));
}

void InputHandler::OnKeyUp(const Int32 key)
{
	mKeyboardState.PrevKeyStates[key] = mKeyboardState.CurKeyStates[key];
	mKeyboardState.CurKeyStates[key] = false;

	InputKey& inputKey = mInputKeyRegistry[key];
	KeyboardAction action;
	action.Key = inputKey;
	action.State = mKeyboardState.GetButtonState(key);

	mKeyActionQueue.push(std::move(action));
}

void InputHandler::OnMouseMove(const Int32 xPos, const Int32 yPos)
{
	mMouseState.PrevPosition.SetXY(mMouseState.CurPosition.X(), mMouseState.CurPosition.Y());
	mMouseState.CurPosition.SetXY(static_cast<float>(xPos), static_cast<float>(yPos));

	MouseAction action;
	action.Button = EMouseButton::MouseButton_Move;
	action.Position = mMouseState.CurPosition;
	action.State = EButtonState::ButtonState_Pressed;
	action.Wheel = 0;

	if (mMouseState.CurPosition != mMouseState.PrevPosition)
	{
		mMouseActionQueue.push(std::move(action));
	}
}

void InputHandler::OnMouseWheel(const Int32 value)
{
	mMouseState.PrevWheelVal = mMouseState.CurWheelVal;
	mMouseState.CurWheelVal = value;

	// This may give slightly incorrect values, 
	// should keep an eye here to see if we should log the position coming in from windows for mouse wheels
	MouseAction action;
	action.Button = EMouseButton::MouseButton_Move;
	action.State = EButtonState::ButtonState_Pressed;
	action.Position = mMouseState.CurPosition;
	action.Wheel = value;

	mMouseActionQueue.push(std::move(action));
}

void InputHandler::OnMouseDown(const EMouseButton::T button, const Int32 xPos, const Int32 yPos)
{
	mMouseState.PrevMouseBtnStates[button] = mMouseState.CurMouseBtnStates[button];
	mMouseState.CurMouseBtnStates[button] = true;

	MouseAction action;
	action.Button = button;
	action.State = mMouseState.GetButtonState(button);
	action.Position = std::move(Vector2D(xPos, yPos));
	action.Wheel = 0;

	mMouseActionQueue.push(std::move(action));
}

void InputHandler::OnMouseUp(const EMouseButton::T button, const Int32 xPos, const Int32 yPos)
{
	mMouseState.PrevMouseBtnStates[button] = mMouseState.CurMouseBtnStates[button];
	mMouseState.CurMouseBtnStates[button] = false;

	MouseAction action;
	action.Button = button;
	action.State = mMouseState.GetButtonState(button);
	action.Position = std::move(Vector2D(xPos, yPos));
	action.Wheel = 0;

	mMouseActionQueue.push(std::move(action));
}

void InputHandler::RaiseKeyEvent(const InputKey& inputKey)
{
	EButtonState::T buttonState = mKeyboardState.GetButtonState(inputKey.GetKeyCode());
	auto& bindingIt = mKeyboardBindings.find(inputKey.GetKeyCode());
	if (bindingIt != mKeyboardBindings.end())
	{
	 	KeyboardActionBinding& actionBinding = (*bindingIt).second;
	 	if (actionBinding.Action.State == buttonState)
	 	{
	 		actionBinding.Func(inputKey);
	 	}
	}
}

void InputHandler::RaiseMouseAxisEvent(const Vector2D& axis, Int32 wheel)
{
	for (auto& binding : mAxisBindings[EAxisBinding::AxisBinding_Mouse])
	{
		binding.Func(Vector3D(axis.X(), axis.Y(), 0.0f), wheel);
	}
}

void InputHandler::RaiseMouseButtonEvent(const EMouseButton::T button, const EButtonState::T state, Int32 xPos, const Int32 yPos)
{
	for (auto& binding : mMouseBtnBindings[button])
	{ 
		if (binding.ButtonState == state)
		{
			binding.Func(xPos, yPos);
		}
	}
}
