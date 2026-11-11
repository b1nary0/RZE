#pragma once

class Window;

// ImGui's platform side for the application's window: its size, the mouse cursor's shape, the clipboard and IME. The
// keyboard and mouse reach ImGui through ImGuiInput, from the input handler, not from here.
namespace ImGuiPlatform
{
	// Once ImGui's context exists
	void Initialize(Window& window);
	// Each UI frame, before ImGui::NewFrame
	void NewFrame();
	// Before ImGui's context is destroyed
	void Shutdown();
}
