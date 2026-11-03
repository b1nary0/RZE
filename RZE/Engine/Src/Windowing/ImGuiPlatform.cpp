#include <StdAfx.h>

#include <Windowing/ImGuiPlatform.h>

#include <Windowing/Window.h>

#include <imgui/imgui_impl_sdl3.h>

#include <SDL3/SDL.h>

namespace ImGuiPlatform
{
	void Initialize(Window& window)
	{
		ImGui_ImplSDL3_InitForOther(static_cast<SDL_Window*>(window.GetNativeHandle()));
		// No gamepad, as before
		ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
		// The backend turns SDL's own mouse capture off, to capture the mouse itself from the events it's given. It's
		// given none (ImGuiInput feeds ImGui), so SDL's is turned back on: the window keeps the mouse while a button is
		// down, so a drag that ends past its edge still ends.
		SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "1");
	}

	void NewFrame()
	{
		ImGui_ImplSDL3_NewFrame();
	}

	void Shutdown()
	{
		ImGui_ImplSDL3_Shutdown();
	}
}
