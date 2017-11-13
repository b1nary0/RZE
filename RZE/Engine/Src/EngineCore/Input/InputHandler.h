#pragma once

#include <unordered_map>

#include <EngineCore/Input/InputCore.h>

#include <Utils/Functor.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;

class InputHandler
{
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

	void TestBindAction(Int32 keyCode, EButtonState::T buttonState, Functor<void, const InputKey&> func);

	void OnKeyDown(const Int32 key, bool bIsRepeat);
	void OnKeyUp(const Int32 key, bool bIsRepeat);

private:
	std::vector<InputKey> mInputKeyRegistry;

	std::unordered_map<Int32, KeyboardActionBinding> mKeyboardBindings;
};
