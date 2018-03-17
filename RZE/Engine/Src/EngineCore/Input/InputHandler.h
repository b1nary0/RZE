#pragma once

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
	struct KeyboardState
	{
		std::bitset<MAX_KEYCODES_SUPPORTED> CurKeyStates;
		std::bitset<MAX_KEYCODES_SUPPORTED> PrevKeyStates;

		void Reset()
		{
			CurKeyStates.reset();
			PrevKeyStates.reset();
		}

		EButtonState::T GetButtonState(Int32 key) const
		{
			EButtonState::T buttonState = CurKeyStates[key] ? EButtonState::ButtonState_Pressed : EButtonState::ButtonState_Released;
			if (CurKeyStates[key] && PrevKeyStates[key])
			{
				buttonState = EButtonState::ButtonState_Hold;
			}

			return buttonState;
		}
	};

	struct MouseState
	{
		std::bitset<MAX_MOUSE_BTN_SUPPORTED> CurMouseBtnStates;
		std::bitset<MAX_MOUSE_BTN_SUPPORTED> PrevMouseBtnStates;

		Vector2D CurPosition;
		Vector2D PrevPosition;
		Int32 CurWheelVal;
		Int32 PrevWheelVal;

		void Reset()
		{
			CurMouseBtnStates.reset();
			PrevMouseBtnStates.reset();
		}

		EButtonState::T GetButtonState(EMouseButton::T button) const
		{
			EButtonState::T buttonState = CurMouseBtnStates[button] ? EButtonState::ButtonState_Pressed : EButtonState::ButtonState_Released;
			if (CurMouseBtnStates[button] && PrevMouseBtnStates[button])
			{
				buttonState = EButtonState::ButtonState_Hold;
			}

			return buttonState;
		}
	};

	struct KeyboardAction
	{
		EButtonState::T State;
		InputKey Key;
	};

	struct ActionBinding
	{
		char* ActionName;
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

	void RaiseEvents();

	void BindAction(Int32 keyCode, EButtonState::T buttonState, KeyActionFunc func);
	void BindAxis(EAxisBinding::T bindingType, EAxisType::T axisType, Functor<void, const Vector3D&, Int32> func);
	void BindMouseAction(EMouseButton::T button, EButtonState::T state, MouseActionFunc func);

	void OnKeyDown(const Int32 key, bool bIsRepeat);
	void OnKeyUp(const Int32 key);

	void OnMouseMove(const Int32 xPos, const Int32 yPos);
	void OnMouseWheel(const Int32 value);
	void OnMouseDown(const EMouseButton::T button, const Int32 xPos, const Int32 yPos);
	void OnMouseUp(const EMouseButton::T button, const Int32 xPos, const Int32 yPos);

	const MouseState& GetMouseState() { return mMouseState; }
	const KeyboardState& GetKeyboardState() { return mKeyboardState; }

private:
	void RaiseKeyEvent(const InputKey& inputKey);
	void RaiseMouseAxisEvent(const Vector2D& axis, Int32 wheel);
	void RaiseMouseButtonEvent(const EMouseButton::T button, const EButtonState::T state, const Int32 xPos, const Int32 yPos);

	// Using declarations for easier reading of member variable types
public:
	using KeyboardBindingMap = std::unordered_map<Int32, KeyboardActionBinding>;
	using AxisBindingMap = std::unordered_map<EAxisBinding::T, std::vector<AxisBinding>>;
	using MouseBindingMap = std::unordered_map <EMouseButton::T, std::vector<MouseButtonBinding>>;

private:
	std::vector<InputKey> mInputKeyRegistry;

	KeyboardBindingMap mKeyboardBindings;
	AxisBindingMap mAxisBindings;
	// The below should change when I implement a proper mouse button enum. Just did this for quick test implementation
	MouseBindingMap mMouseBtnBindings;

private:
	std::queue<KeyboardAction> mKeyActionQueue;
	std::queue<MouseAction> mMouseActionQueue;

	KeyboardState mKeyboardState;
	MouseState mMouseState;
};
