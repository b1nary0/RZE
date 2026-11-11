#pragma once

#include <memory>

// What a Window keeps of its native side (WindowNative.h), apart from the functions that use it, so Window.h can hold it
namespace WindowNative
{
	// Per platform: whatever it keeps for a window
	struct State;

	// Frees a State before its window goes. Per platform, like State itself, which is only complete there.
	struct StateDeleter
	{
		void operator()(State* state) const;
	};

	using StatePtr = std::unique_ptr<State, StateDeleter>;
}
