#include <StdAfx.h>

#include <Windowing/WindowNative.h>

#include <Windowing/WindowIcon.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

#include <SDL3/SDL.h>

#include <algorithm>

struct WindowNative::State
{
	SDL_Window* SdlWindow = nullptr;
	const bool* HasCustomTitleBar = nullptr;
	const TitleBarLayout* Layout = nullptr;
};

namespace
{
	// Windows' metrics at 96 DPI, so a custom title bar is the size it is there: the caption (SM_CYCAPTION), and the
	// resize border (SM_CYSIZEFRAME + SM_CXPADDEDBORDER). The window isn't scaled for the display (it has no high pixel
	// density), as a DPI-unaware window isn't on Windows, so neither are these.
	constexpr int k_captionHeight = 23;
	constexpr int k_frameThickness = 8;

	// The largest icon made from an image; the desktop scales it down as it needs
	constexpr int k_maxIconSize = 256;

	// What the mouse is over, for the compositor to move or resize the window (a custom title bar's window has no
	// decorations of its own). The resize bands lie inside the client area, along its sides and bottom.
	SDL_HitTestResult SDLCALL HitTest(SDL_Window* window, const SDL_Point* point, void* stateData)
	{
		const WindowNative::State& state = *static_cast<const WindowNative::State*>(stateData);

		int width = 0;
		int height = 0;
		SDL_GetWindowSize(window, &width, &height);
		const bool isMaximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;

		switch (state.Layout->HitTest(point->x, point->y, width, isMaximized, k_frameThickness))
		{
		case ETitleBarHit::Caption:
			return SDL_HITTEST_DRAGGABLE;
		case ETitleBarHit::ResizeTop:
			return SDL_HITTEST_RESIZE_TOP;
		case ETitleBarHit::ResizeTopLeft:
			return SDL_HITTEST_RESIZE_TOPLEFT;
		case ETitleBarHit::ResizeTopRight:
			return SDL_HITTEST_RESIZE_TOPRIGHT;
		// The application's own button
		case ETitleBarHit::MaximizeButton:
			return SDL_HITTEST_NORMAL;
		case ETitleBarHit::Client:
		default:
			break;
		}

		// No resizing while it's maximized, as on Windows
		if (isMaximized)
		{
			return SDL_HITTEST_NORMAL;
		}

		const bool isLeft = point->x < k_frameThickness;
		const bool isRight = point->x >= width - k_frameThickness;
		const bool isBottom = point->y >= height - k_frameThickness;
		if (isBottom)
		{
			return isLeft ? SDL_HITTEST_RESIZE_BOTTOMLEFT : (isRight ? SDL_HITTEST_RESIZE_BOTTOMRIGHT : SDL_HITTEST_RESIZE_BOTTOM);
		}
		if (isLeft)
		{
			return SDL_HITTEST_RESIZE_LEFT;
		}
		if (isRight)
		{
			return SDL_HITTEST_RESIZE_RIGHT;
		}
		return SDL_HITTEST_NORMAL;
	}
}

namespace WindowNative
{
	StatePtr CreateState(SDL_Window* window, const Display::ScreenRect& workArea, int width, int height, const bool& hasCustomTitleBar, const TitleBarLayout& titleBarLayout)
	{
		StatePtr state(new State());
		state->SdlWindow = window;
		state->HasCustomTitleBar = &hasCustomTitleBar;
		state->Layout = &titleBarLayout;

		// The compositor draws the frame, so width x height is the client area's size. The position is only kept where
		// the desktop lets an application place its windows (X11, not Wayland).
		(void)width;
		(void)height;
		SDL_SetWindowPosition(window, workArea.Left, workArea.Top);
		return state;
	}

	void StateDeleter::operator()(State* state) const
	{
		SDL_SetWindowHitTest(state->SdlWindow, nullptr, nullptr);
		delete state;
	}

	void Show(State& /*state*/)
	{
		// Left hidden until Reveal: there's no way to show a window the user doesn't see yet
	}

	void Reveal(State& state)
	{
		SDL_ShowWindow(state.SdlWindow);
	}

	void SetSize(State& state, int width, int height)
	{
		SDL_SetWindowSize(state.SdlWindow, width, height);
	}

	void GetClientSize(State& state, int& outWidth, int& outHeight)
	{
		SDL_GetWindowSizeInPixels(state.SdlWindow, &outWidth, &outHeight);
	}

	void OnCustomTitleBarChanged(State& state)
	{
		const bool isCustom = *state.HasCustomTitleBar;
		// No decorations at all; the hit test gives back moving and resizing
		SDL_SetWindowBordered(state.SdlWindow, !isCustom);
		SDL_SetWindowHitTest(state.SdlWindow, isCustom ? &HitTest : nullptr, &state);
	}

	int GetSystemTitleBarHeight(State& /*state*/)
	{
		return k_captionHeight + k_frameThickness;
	}

	float GetDpiScale(State& state)
	{
		// 1: the window isn't scaled for the display (see k_captionHeight)
		return SDL_GetWindowPixelDensity(state.SdlWindow);
	}

	bool SetIcon(State& state, const Filepath& image)
	{
		const IconImage source = WindowIcon::ReadImage(image);
		if (source.IsEmpty())
		{
			RZE_LOG_WARNING_ARGS("Couldn't read the window icon [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
			return false;
		}

		IconImage square = WindowIcon::MakeSquare(source, std::min(k_maxIconSize, std::max(source.Width, source.Height)));
		// RGBA bytes, as the texels are; SDL copies them
		SDL_Surface* const surface = SDL_CreateSurfaceFrom(square.Width, square.Height, SDL_PIXELFORMAT_RGBA32, square.Texels.data(), square.Width * 4);
		const bool isSet = surface != nullptr && SDL_SetWindowIcon(state.SdlWindow, surface);
		SDL_DestroySurface(surface);
		if (!isSet)
		{
			RZE_LOG_WARNING_ARGS("Couldn't make the window icon from [%s]; keeping the executable's icon.", image.GetRelativePath().c_str());
		}
		return isSet;
	}

	bool HandleEvent(State& /*state*/, const SDL_Event& /*event*/, InputHandler& /*inputHandler*/)
	{
		return false;
	}
}
