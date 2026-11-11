#pragma once

// What a Window needs from the OS that SDL doesn't give it the same way on every platform: the custom title bar's
// hit-testing, frame metrics, icons, and how the window first appears. Only Window.cpp uses it. Per platform in
// Windowing/<platform>/WindowNative<platform>.cpp.

#include <Windowing/Display.h>
#include <Windowing/TitleBarLayout.h>
#include <Windowing/WindowNativeState.h>

class Filepath;
class InputHandler;

struct SDL_Window;
union SDL_Event;

namespace WindowNative
{
	// Once SDL has made window, hidden and not yet placed: puts it at the top-left of workArea, its outer size
	// width x height where the OS lets the application set that (its client size otherwise), and readies it to be
	// shown. hasCustomTitleBar and titleBarLayout are the window's, read on the main thread whenever the OS asks; they
	// must outlive the returned state, which must be freed before the window goes.
	StatePtr CreateState(SDL_Window* window, const Display::ScreenRect& workArea, int width, int height, const bool& hasCustomTitleBar, const TitleBarLayout& titleBarLayout);

	// Shown, as the application sees it; the user doesn't see it until Reveal
	void Show(State& state);
	// Now the user sees it
	void Reveal(State& state);

	// Sets the window's size as CreateState does
	void SetSize(State& state, int width, int height);
	// The client area, in pixels
	void GetClientSize(State& state, int& outWidth, int& outHeight);

	// After the window's hasCustomTitleBar changes
	void OnCustomTitleBarChanged(State& state);
	// The height of the OS's title bar, for a custom one to match
	int GetSystemTitleBarHeight(State& state);
	// The window's DPI over the standard 96, e.g. 1.25
	float GetDpiScale(State& state);

	// False if image couldn't be read, which leaves the icon as it was
	bool SetIcon(State& state, const Filepath& image);

	// An SDL event only the native side knows (e.g. the mouse over the custom title bar): passes it to inputHandler and
	// returns true. False for every other event.
	bool HandleEvent(State& state, const SDL_Event& event, InputHandler& inputHandler);
}
