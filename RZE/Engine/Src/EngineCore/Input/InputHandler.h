#pragma once

#include <unordered_map>
#include <bitset>

#include <EngineCore/Input/InputCore.h>

#include <Utils/Functor.h>
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

	struct ActionBinding
	{
		char* ActionName;
		EButtonState::T ButtonState;
	};

	struct KeyboardActionBinding : public ActionBinding
	{
		Functor<void, const InputKey&> Func;
	};

public:
	InputHandler();

	void Initialize();

	void BindAction(Int32 keyCode, EButtonState::T buttonState, Functor<void, const InputKey&> func);

	void OnKeyDown(const Int32 key, bool bIsRepeat);
	void OnKeyUp(const Int32 key);

private:
	void RaiseKeyEvent(const InputKey& inputKey);

private:
	std::vector<InputKey> mInputKeyRegistry;
	std::unordered_map<Int32, KeyboardActionBinding> mKeyboardBindings;

private:
	KeyboardState mKeyboardState;
};
