#pragma once

class InputHandler;

namespace ImGuiInput
{
	// Forwards the input handler's proxy mouse/keyboard state to ImGui through the IO event API.
	void SubmitInput(const InputHandler& handler);
}
