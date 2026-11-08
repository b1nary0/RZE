#include <EngineCore/Input/InputHandler.h>

#include <DebugUtils/DebugServices.h>

#include <Events/EventHandler.h>

InputHandler::InputHandler()
{
}

void InputHandler::Initialize()
{
	mProxyKeyboardState.Reset();

	mInputKeyRegistry.reserve(MAX_KEYCODES_SUPPORTED);
	for (U32 keyCodeIdx = 0; keyCodeIdx < MAX_KEYCODES_SUPPORTED; keyCodeIdx++)
	{
		mInputKeyRegistry.insert({keyCodeIdx, InputKey(static_cast<unsigned char>(keyCodeIdx), keyCodeIdx)});
	}
}

void InputHandler::RaiseEvents()
{
	// Prev becomes last frame's Cur, so GetButtonState and WasDownLastFrame count frames, however many events came
	mKeyboardState.PrevKeyStates = mKeyboardState.CurKeyStates;
	mKeyboardState.CurKeyStates = mProxyKeyboardState.CurKeyStates;

	const std::bitset<MAX_MOUSE_BTN_SUPPORTED> lastFrameMouseBtnStates = mMouseState.CurMouseBtnStates;
	mMouseState = mProxyMouseState;
	mMouseState.PrevMouseBtnStates = lastFrameMouseBtnStates;

	while (!mKeyActionQueue.empty())
	{
		KeyboardAction action = mKeyActionQueue.front();
		mKeyActionQueue.pop();

		AssertExpr(action.Key.IsValid());
		RaiseKeyEvent(action.Key, action.State);
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
			RaiseMouseButtonEvent(action.Button, action.State, static_cast<Int32>(action.Position.X()), static_cast<Int32>(action.Position.Y()));
		}
	}
}

void InputHandler::BindAction(Int32 keyCode, EButtonState::T buttonState, KeyActionFunc func)
{
	KeyboardActionBinding actionBinding;
	actionBinding.ActionName = "Test";
	actionBinding.Func = std::move(func);

	actionBinding.State = buttonState;

	mKeyboardBindings[keyCode] = actionBinding;
}

void InputHandler::BindAxis(EAxisBinding::T bindingType, EAxisType::T axisType, Functor<void, const Vector3D&, Int32> func)
{
	std::vector<AxisBinding>& bindings = mAxisBindings[bindingType];
	
	AxisBinding& binding = bindings.emplace_back();
	binding.AxisType = axisType;
	binding.BindingType = bindingType;
	binding.Func = func;
}

void InputHandler::BindMouseAction(EMouseButton::T button, EButtonState::T state, MouseActionFunc func)
{
	mMouseBtnBindings[button].emplace_back(button, state, func);
}

void InputHandler::BeginFrame()
{
	mKeysPressedThisFrame.reset();
	mMouseBtnsPressedThisFrame.reset();

	const std::bitset<MAX_KEYCODES_SUPPORTED> deferredKeyReleases = mDeferredKeyReleases;
	mDeferredKeyReleases.reset();
	for (Int32 key = 0; key < MAX_KEYCODES_SUPPORTED; ++key)
	{
		if (deferredKeyReleases[key])
		{
			ReleaseKey(key);
		}
	}

	const std::bitset<MAX_MOUSE_BTN_SUPPORTED> deferredMouseBtnReleases = mDeferredMouseBtnReleases;
	mDeferredMouseBtnReleases.reset();
	for (Int32 button = 0; button < MAX_MOUSE_BTN_SUPPORTED; ++button)
	{
		if (deferredMouseBtnReleases[button])
		{
			const Vector2D& position = mDeferredMouseReleasePositions[button];
			ReleaseMouseButton(static_cast<EMouseButton::T>(button), static_cast<Int32>(position.X()), static_cast<Int32>(position.Y()));
		}
	}
}

void InputHandler::OnKeyDown(const Int32 key, bool isRepeat)
{
	mProxyKeyboardState.CurKeyStates[key] = true;
	// Down again before its release was let go: it's held, so that release no longer happens
	mDeferredKeyReleases[key] = false;
	if (!isRepeat)
	{
		mKeysPressedThisFrame[key] = true;
	}

	KeyboardAction action;
	action.Key = mInputKeyRegistry[key];
	action.State = isRepeat ? EButtonState::ButtonState_Hold : EButtonState::ButtonState_Pressed;

	mKeyActionQueue.push(std::move(action));
	CountInputEvent();
}

void InputHandler::OnKeyUp(const Int32 key)
{
	if (mKeysPressedThisFrame[key])
	{
		mDeferredKeyReleases[key] = true;
		return;
	}

	ReleaseKey(key);
}

void InputHandler::ReleaseKey(Int32 key)
{
	mProxyKeyboardState.CurKeyStates[key] = false;

	KeyboardAction action;
	action.Key = mInputKeyRegistry[key];
	action.State = EButtonState::ButtonState_Released;

	mKeyActionQueue.push(std::move(action));
	CountInputEvent();
}

void InputHandler::OnMouseMove(const Int32 xPos, const Int32 yPos)
{
	mProxyMouseState.PrevPosition.SetXY(mProxyMouseState.CurPosition.X(), mProxyMouseState.CurPosition.Y());
	mProxyMouseState.CurPosition.SetXY(static_cast<float>(xPos), static_cast<float>(yPos));

	MouseAction action;
	action.Button = EMouseButton::MouseButton_Move;
	action.Position = mProxyMouseState.CurPosition;
	action.State = EButtonState::ButtonState_Pressed;
	action.Wheel = 0;

	if (mProxyMouseState.CurPosition != mProxyMouseState.PrevPosition)
	{
		mMouseActionQueue.push(std::move(action));
		CountInputEvent();
	}
}

void InputHandler::OnMouseWheel(const Int32 value)
{
	// Adds up: every queued message is handled each frame, so one frame can bring several wheel messages
	mProxyMouseState.PrevWheelVal = mProxyMouseState.CurWheelVal;
	mProxyMouseState.CurWheelVal += value;

	// This may give slightly incorrect values, 
	// should keep an eye here to see if we should log the position coming in from windows for mouse wheels
	MouseAction action;
	action.Button = EMouseButton::MouseButton_Move;
	action.State = EButtonState::ButtonState_Pressed;
	action.Position = mProxyMouseState.CurPosition;
	action.Wheel = value;

	mMouseActionQueue.push(std::move(action));
	if (value != 0)
	{
		CountInputEvent();
	}
}

void InputHandler::ClearMouseWheel()
{
	mProxyMouseState.PrevWheelVal = mProxyMouseState.CurWheelVal;
	mProxyMouseState.CurWheelVal = 0;
}

void InputHandler::OnMouseDown(const EMouseButton::T button, const Int32 xPos, const Int32 yPos)
{
	mProxyMouseState.CurMouseBtnStates[button] = true;
	// Down again before its release was let go: it's held, so that release no longer happens
	mDeferredMouseBtnReleases[button] = false;
	mMouseBtnsPressedThisFrame[button] = true;

	MouseAction action;
	action.Button = button;
	action.State = EButtonState::ButtonState_Pressed;
	action.Position = Vector2D(xPos, yPos);
	action.Wheel = 0;

	mMouseActionQueue.push(std::move(action));
	CountInputEvent();
}

void InputHandler::OnMouseUp(const EMouseButton::T button, const Int32 xPos, const Int32 yPos)
{
	if (mMouseBtnsPressedThisFrame[button])
	{
		mDeferredMouseBtnReleases[button] = true;
		mDeferredMouseReleasePositions[button] = Vector2D(xPos, yPos);
		return;
	}

	ReleaseMouseButton(button, xPos, yPos);
}

void InputHandler::ReleaseMouseButton(EMouseButton::T button, Int32 xPos, Int32 yPos)
{
	mProxyMouseState.CurMouseBtnStates[button] = false;

	MouseAction action;
	action.Button = button;
	action.State = EButtonState::ButtonState_Released;
	action.Position = Vector2D(xPos, yPos);
	action.Wheel = 0;

	mMouseActionQueue.push(std::move(action));
	CountInputEvent();
}

void InputHandler::ReleaseAll()
{
	// Deferred releases are of keys and buttons still down, so the loops below let go of them too
	mDeferredKeyReleases.reset();
	mDeferredMouseBtnReleases.reset();

	for (Int32 key = 0; key < MAX_KEYCODES_SUPPORTED; ++key)
	{
		if (mProxyKeyboardState.CurKeyStates[key])
		{
			ReleaseKey(key);
		}
	}

	const Int32 xPos = static_cast<Int32>(mProxyMouseState.CurPosition.X());
	const Int32 yPos = static_cast<Int32>(mProxyMouseState.CurPosition.Y());
	for (Int32 button = 0; button < MAX_MOUSE_BTN_SUPPORTED; ++button)
	{
		if (mProxyMouseState.CurMouseBtnStates[button])
		{
			ReleaseMouseButton(static_cast<EMouseButton::T>(button), xPos, yPos);
		}
	}
}

void InputHandler::UpdateFrameState(bool isWindowFocused, bool isMouseButtonDown)
{
	if (mWasWindowFocused && !isWindowFocused)
	{
		ReleaseAll();
	}
	mWasWindowFocused = isWindowFocused;

	const bool hadInput = mInputEventCount != mLastFrameInputEventCount;
	mLastFrameInputEventCount = mInputEventCount;

	mIsInteracting = hadInput || (isWindowFocused && isMouseButtonDown);
}

void InputHandler::CountInputEvent()
{
	++mInputEventCount;
}

void InputHandler::Reset()
{
	std::queue<MouseAction>().swap(mMouseActionQueue);
	std::queue<KeyboardAction>().swap(mKeyActionQueue);

	mMouseState.Reset();
	mKeyboardState.Reset();

	// Keep the cursor position current while input is gated, otherwise anything tracking
	// per-frame mouse deltas sees a jump from the stale position once input resumes.
	mMouseState.CurPosition = mProxyMouseState.CurPosition;
	mMouseState.PrevPosition = mProxyMouseState.PrevPosition;
}

void InputHandler::RaiseKeyEvent(const InputKey& inputKey, EButtonState::T state)
{
	auto bindingIt = mKeyboardBindings.find(inputKey.GetKeyCode());
	if (bindingIt != mKeyboardBindings.end())
	{
	 	KeyboardActionBinding& actionBinding = (*bindingIt).second;
	 	if (actionBinding.State == state)
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
