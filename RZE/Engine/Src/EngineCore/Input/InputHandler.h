#pragma once

#include <array>
#include <bitset>
#include <queue>
#include <unordered_map>

#include <EngineCore/Input/InputCore.h>

#include <Utils/Functor.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;

class InputHandler
{
public:
	typedef Functor<void, const Int32, const Int32> MouseActionFunc;
	typedef Functor<void, const InputKey&> KeyActionFunc;

private:
	// Cur is what's down now. Prev is what was down the frame before; only the states the app reads each frame
	// (GetKeyboardState, GetMouseState) keep it, not the proxies.
	struct KeyboardState
	{
		std::bitset<MAX_KEYCODES_SUPPORTED> CurKeyStates;
		std::bitset<MAX_KEYCODES_SUPPORTED> PrevKeyStates;

		void Reset()
		{
			CurKeyStates.reset();
			PrevKeyStates.reset();
		}

		// Pressed the first frame it's down, Hold from the second, Released while it's up
		EButtonState::T GetButtonState(Int32 key) const
		{
			EButtonState::T buttonState = CurKeyStates[key] ? EButtonState::ButtonState_Pressed : EButtonState::ButtonState_Released;
			if (CurKeyStates[key] && PrevKeyStates[key])
			{
				buttonState = EButtonState::ButtonState_Hold;
			}

			return buttonState;
		}

		bool IsDownThisFrame(Int32 keyCode) const
		{
			return CurKeyStates[keyCode];
		}

		bool WasDownLastFrame(Int32 keyCode) const
		{
			return PrevKeyStates[keyCode];
		}

		// None pressed
		bool IsIdle() const
		{
			std::bitset<MAX_KEYCODES_SUPPORTED> idleState(0);
			return (CurKeyStates | idleState).none();
		}
	};

	struct MouseState
	{
		std::bitset<MAX_MOUSE_BTN_SUPPORTED> CurMouseBtnStates;
		std::bitset<MAX_MOUSE_BTN_SUPPORTED> PrevMouseBtnStates;

		Vector2D CurPosition;
		// Last frame's position, in the state the app reads (GetMouseState); the proxies leave it unset. Equal to
		// CurPosition while the app doesn't take the mouse, and in the frame the window first reports a position.
		Vector2D PrevPosition;
		// This frame's wheel turns, added up (k_mouseWheelNotchDelta per notch)
		Int32 CurWheelVal = 0;

		void Reset()
		{
			CurMouseBtnStates.reset();
			PrevMouseBtnStates.reset();
		}

		// Pressed the first frame it's down, Hold from the second, Released while it's up
		EButtonState::T GetButtonState(EMouseButton::T button) const
		{
			EButtonState::T buttonState = CurMouseBtnStates[button] ? EButtonState::ButtonState_Pressed : EButtonState::ButtonState_Released;
			if (CurMouseBtnStates[button] && PrevMouseBtnStates[button])
			{
				buttonState = EButtonState::ButtonState_Hold;
			}

			return buttonState;
		}

		bool IsDownThisFrame(EMouseButton::T button) const
		{
			return CurMouseBtnStates[button];
		}

		// How far the pointer moved since last frame, in client pixels
		Vector2D GetPositionDelta() const { return CurPosition - PrevPosition; }

		bool IsIdle() const
		{
			std::bitset<MAX_MOUSE_BTN_SUPPORTED> idleState(0);
			return (CurMouseBtnStates | idleState).none();
		}
	};

	struct KeyboardAction
	{
		EButtonState::T State;
		InputKey Key;
	};

	struct ActionBinding
	{
		std::string ActionName;
	};

	struct KeyboardActionBinding : public ActionBinding
	{
		EButtonState::T State;
		KeyActionFunc Func;
	};

	struct AxisBinding
	{
		EAxisBinding::T BindingType;
		EAxisType::T AxisType;
		Vector3D Axis;
		float Wheel;
		Functor<void, const Vector3D&, Int32> Func;
	};
	
	struct MouseAction
	{
		Vector2D Position;
		EMouseButton::T Button;
		EButtonState::T State;
		int Wheel;
	};

	struct MouseButtonBinding
	{
		MouseButtonBinding(EMouseButton::T button, EButtonState::T state, MouseActionFunc func)
			: Button(button)
			, ButtonState(state)
			, Func(func) {}

		EMouseButton::T Button;
		EButtonState::T ButtonState;
		MouseActionFunc Func;
	};

public:
	InputHandler();

public:
	void Initialize();

	// Once a frame: gives the app the input of each device the gate lets through (its state, and its bindings fire),
	// and clears the others' (Reset, for that device)
	void RaiseEvents(const InputGate& gate);

	void BindAction(Int32 keyCode, EButtonState::T buttonState, KeyActionFunc func);
	void BindAxis(EAxisBinding::T bindingType, EAxisType::T axisType, Functor<void, const Vector3D&, Int32> func);
	void BindMouseAction(EMouseButton::T button, EButtonState::T state, MouseActionFunc func);

	// At the start of each frame's input, before the window's messages. Lets go of the keys and buttons released in
	// the frame they were pressed in (see OnKeyUp).
	void BeginFrame();

	// isRepeat: the OS repeating a held key, which raises Hold rather than Pressed
	void OnKeyDown(const Int32 key, bool isRepeat);
	// A key released in the same frame it was pressed in stays down until the next BeginFrame, so the app sees every
	// press for at least a frame. That happens with fast typing at a low frame rate, or when the OS hands over a press
	// and its release together, as KDE does for keys that type text while text input is on. OnMouseUp does the same.
	void OnKeyUp(const Int32 key);

	void OnMouseMove(const Int32 xPos, const Int32 yPos);
	void OnMouseWheel(const Int32 value);
	// The wheel only reports deltas: the window clears it at the start of each frame, before that frame's messages
	void ClearMouseWheel();
	void OnMouseDown(const EMouseButton::T button, const Int32 xPos, const Int32 yPos);
	void OnMouseUp(const EMouseButton::T button, const Int32 xPos, const Int32 yPos);
	// Lets go of every key and button still down, as if each was released, e.g. when the window loses focus and the
	// releases would go to another window. At once, even those pressed this frame.
	void ReleaseAll();

	// Once a frame, after the window's messages. When the window has just lost focus, lets go of everything still down
	// (ReleaseAll), since the releases will go to another window. Then works out IsInteracting. isMouseButtonDown is the
	// OS's view, which unlike this handler's state can't get stuck down when a button is released outside the window.
	void UpdateFrameState(bool isWindowFocused, bool isMouseButtonDown);
	// As of UpdateFrameState: whether this frame had input (a held key repeats, so it counts), or a mouse button is held
	// while the window is focused
	bool IsInteracting() const { return mIsInteracting; }

	const MouseState& GetMouseState() const { return mMouseState; }
	const KeyboardState& GetKeyboardState() const { return mKeyboardState; }

	// How many input events (keys, buttons, wheel turns, and mouse moves that changed the position) the window has given
	// the handler so far. It only goes up, so a change since last time means there was input in between.
	U64 GetInputEventCount() const { return mInputEventCount; }

	// The input as the window reports it, updated as events arrive. Most code should read GetMouseState and
	// GetKeyboardState, the snapshot the app sees each frame. These are for readers that must see every event
	// whatever the app is shown, such as ImGui's input and the window's own handling.
	const MouseState& GetProxyMouseState() const { return mProxyMouseState; }
	const KeyboardState& GetProxyKeyboardState() const { return mProxyKeyboardState; }

	// Drops both devices' queued events and clears the state the app reads, e.g. while there's no window to take input
	void Reset();

private:
	void RaiseKeyboardEvents();
	void RaiseMouseEvents();
	void ResetKeyboard();
	void ResetMouse();

	// One more for GetInputEventCount: the window gave the handler real input (a key, a button, a wheel turn, a move)
	void CountInputEvent();

	// Lets go at once, whichever frame it was pressed in
	void ReleaseKey(Int32 key);
	void ReleaseMouseButton(EMouseButton::T button, Int32 xPos, Int32 yPos);

	void RaiseKeyEvent(const InputKey& inputKey, EButtonState::T state);
	void RaiseMouseAxisEvent(const Vector2D& axis, Int32 wheel);
	void RaiseMouseButtonEvent(const EMouseButton::T button, const EButtonState::T state, const Int32 xPos, const Int32 yPos);

	// Using declarations for easier reading of member variable types
public:
	using KeyboardBindingMap = std::unordered_map<Int32, KeyboardActionBinding>;
	using AxisBindingMap = std::unordered_map<EAxisBinding::T, std::vector<AxisBinding>>;
	using MouseBindingMap = std::unordered_map <EMouseButton::T, std::vector<MouseButtonBinding>>;

private:
	std::unordered_map<U8, InputKey> mInputKeyRegistry;

	KeyboardBindingMap mKeyboardBindings;
	AxisBindingMap mAxisBindings;
	// The below should change when I implement a proper mouse button enum. Just did this for quick test implementation
	MouseBindingMap mMouseBtnBindings;

private:
	std::queue<KeyboardAction> mKeyActionQueue;
	std::queue<MouseAction> mMouseActionQueue;

	U64 mInputEventCount = 0;
	// For UpdateFrameState: the event count and focus as of the last frame
	U64 mLastFrameInputEventCount = 0;
	bool mWasWindowFocused = false;
	bool mIsInteracting = false;

	// Double-buffered: the proxy states track the OS's input as it arrives, and RaiseEvents copies them into the states
	// the app reads once a frame. So the handler keeps tracking input even while the app isn't given it (e.g. the
	// Editor keeping it from the scene).
	KeyboardState mProxyKeyboardState;
	MouseState mProxyMouseState;

	KeyboardState mKeyboardState;
	MouseState mMouseState;

	// Pressed since BeginFrame, and released since then too: they're let go at the next BeginFrame (OnKeyUp)
	std::bitset<MAX_KEYCODES_SUPPORTED> mKeysPressedThisFrame;
	std::bitset<MAX_KEYCODES_SUPPORTED> mDeferredKeyReleases;
	std::bitset<MAX_MOUSE_BTN_SUPPORTED> mMouseBtnsPressedThisFrame;
	std::bitset<MAX_MOUSE_BTN_SUPPORTED> mDeferredMouseBtnReleases;
	// Where each deferred button release happened
	std::array<Vector2D, MAX_MOUSE_BTN_SUPPORTED> mDeferredMouseReleasePositions;

	// Whether the window has reported where the pointer is yet, and whether the app's state had that as of its last
	// frame. Until then the position is a placeholder (0, 0), so the first real one isn't a move away from it.
	bool mProxyHasMousePosition = false;
	bool mHasMousePosition = false;
};
