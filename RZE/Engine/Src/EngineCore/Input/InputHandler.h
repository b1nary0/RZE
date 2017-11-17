#pragma once

#include <unordered_map>
#include <bitset>

#include <EngineCore/Input/InputCore.h>

#include <Utils/Functor.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;

class InputHandler
{
	struct KeyboardState
	{
		std::bitset<MAX_KEYCODES_SUPPORTED> CurKeyStates;
		std::bitset<MAX_KEYCODES_SUPPORTED> PrevKeyStates;

		void Reset()
		{
			CurKeyStates.reset();
			PrevKeyStates.reset();
		}

		EButtonState::T GetButtonState(Int32 key)
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

		void Reset()
		{
			CurMouseBtnStates.reset();
			PrevMouseBtnStates.reset();
		}
	};

	struct ActionBinding
	{
		char* ActionName;
		EButtonState::T ButtonState;
	};

	struct KeyboardActionBinding : public ActionBinding
	{
		Functor<void, const InputKey&> Func;
	};

	struct AxisBinding
	{
		Vector3D axis;
	};

public:
	InputHandler();

	void Initialize();

	void BindAction(Int32 keyCode, EButtonState::T buttonState, Functor<void, const InputKey&> func);

	void OnKeyDown(const Int32 key, bool bIsRepeat);
	void OnKeyUp(const Int32 key);
	void OnMouseMove(const Int32 xPos, const Int32 yPos);

private:
	void RaiseKeyEvent(const InputKey& inputKey);
	void RaiseMouseEvent();

private:
	std::vector<InputKey> mInputKeyRegistry;

	std::unordered_map<Int32, KeyboardActionBinding> mKeyboardBindings;

private:
	KeyboardState mKeyboardState;
	MouseState mMouseState;
};
