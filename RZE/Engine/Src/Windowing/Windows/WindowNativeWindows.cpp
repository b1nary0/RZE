#include <StdAfx.h>

#include <Windowing/WindowNative.h>

#include <EngineCore/Input/InputHandler.h>
#include <Windowing/Windows/WindowIconWindows.h>

#include <Utils/DebugUtils/Debug.h>

#include <SDL3/SDL.h>

#include <memory>

#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>

struct WindowNative::State
{
	SDL_Window* SdlWindow = nullptr;
	HWND Handle = nullptr;
	const bool* HasCustomTitleBar = nullptr;
	const TitleBarLayout* Layout = nullptr;
	// Where it was put when made; SetSize keeps it there
	POINT Position = {};
	// The SDL event the non-client mouse messages become (NonClientMouseHook)
	Uint32 NonClientMouseEvent = 0;
	// From SetIcon; null while the window shows the executable's icon
	std::unique_ptr<WindowIconWindows> Icon;
};

namespace
{
	// The executable's icon, from Editor/Resources/Editor.rc and Game/Resources/Game.rc
	constexpr WORD k_appIconResourceID = 1;
	// Its subclass (WindowSubclassProc) among any others the window has
	constexpr UINT_PTR k_subclassID = 1;

	// What a non-client mouse message is, as NonClientMouseHook passes it on in an SDL user event's code
	enum ENonClientMouse : Sint32
	{
		NonClientMouse_Move,
		NonClientMouse_Down,
		NonClientMouse_Up
	};

	// The exe's icon at the given system size, or Windows' default when the exe has none
	HICON LoadAppIcon(int widthMetric, int heightMetric)
	{
		HANDLE icon = LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(k_appIconResourceID), IMAGE_ICON, GetSystemMetrics(widthMetric), GetSystemMetrics(heightMetric), LR_DEFAULTCOLOR);
		return icon != nullptr ? static_cast<HICON>(icon) : LoadIcon(nullptr, IDI_APPLICATION);
	}

	// The invisible resize border's thickness, at the window's DPI
	int GetFrameThickness(HWND windowHandle)
	{
		const UINT dpi = GetDpiForWindow(windowHandle);
		return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
	}

	// With a custom title bar: the client area takes the OS caption's place (WM_NCCALCSIZE)
	LRESULT CalculateClientArea(HWND windowHandle, WPARAM wParam, LPARAM lParam)
	{
		if (wParam == FALSE)
		{
			return DefWindowProc(windowHandle, WM_NCCALCSIZE, wParam, lParam);
		}

		// The OS's sides and bottom stay: they're invisible, and resize the window and cast its shadow. Only the caption
		// goes, by putting the top back where it was.
		NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
		const LONG top = params.rgrc[0].top;
		const LRESULT result = DefWindowProc(windowHandle, WM_NCCALCSIZE, wParam, lParam);
		params.rgrc[0].top = top;

		// A maximized window hangs over the screen's edges by its frame, so the top would be off-screen
		if (IsZoomed(windowHandle))
		{
			params.rgrc[0].top += GetFrameThickness(windowHandle);
		}
		return result;
	}

	// With a custom title bar: the parts of the client area that are the title bar are reported as caption, maximize
	// button or resize border (WM_NCHITTEST)
	LRESULT HitTest(HWND windowHandle, const TitleBarLayout& titleBarLayout, WPARAM wParam, LPARAM lParam)
	{
		// The OS's own borders: the sides and bottom
		const LRESULT osHit = DefWindowProc(windowHandle, WM_NCHITTEST, wParam, lParam);
		if (osHit != HTCLIENT)
		{
			return osHit;
		}

		POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(windowHandle, &point);
		RECT clientRect;
		GetClientRect(windowHandle, &clientRect);

		switch (titleBarLayout.HitTest(point.x, point.y, clientRect.right, IsZoomed(windowHandle) != FALSE, GetFrameThickness(windowHandle)))
		{
		case ETitleBarHit::Caption:
			return HTCAPTION;
		case ETitleBarHit::MaximizeButton:
			return HTMAXBUTTON;
		case ETitleBarHit::ResizeTop:
			return HTTOP;
		case ETitleBarHit::ResizeTopLeft:
			return HTTOPLEFT;
		case ETitleBarHit::ResizeTopRight:
			return HTTOPRIGHT;
		case ETitleBarHit::Client:
		default:
			return HTCLIENT;
		}
	}

	// Runs before SDL's window procedure. SDL makes its window a normal (not borderless) one, so SDL itself leaves
	// WM_NCCALCSIZE and WM_NCHITTEST to DefWindowProc, as this does when there's no custom title bar.
	LRESULT CALLBACK WindowSubclassProc(HWND window, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR stateData)
	{
		const WindowNative::State& state = *reinterpret_cast<const WindowNative::State*>(stateData);
		switch (msg)
		{
		case WM_NCCALCSIZE:
		{
			if (*state.HasCustomTitleBar)
			{
				return CalculateClientArea(window, wp, lp);
			}
		}
		break;

		case WM_NCHITTEST:
		{
			if (*state.HasCustomTitleBar)
			{
				return HitTest(window, *state.Layout, wp, lp);
			}
		}
		break;

		case WM_NCDESTROY:
		{
			RemoveWindowSubclass(window, &WindowSubclassProc, k_subclassID);
		}
		break;
		}
		return DefSubclassProc(window, msg, wp, lp);
	}

	void PushNonClientMouse(const WindowNative::State& state, ENonClientMouse what, POINT clientPoint)
	{
		SDL_Event event = {};
		event.type = state.NonClientMouseEvent;
		event.user.windowID = SDL_GetWindowID(state.SdlWindow);
		event.user.code = what;
		event.user.data1 = reinterpret_cast<void*>(static_cast<intptr_t>(clientPoint.x));
		event.user.data2 = reinterpret_cast<void*>(static_cast<intptr_t>(clientPoint.y));
		SDL_PushEvent(&event);
	}

	// The mouse over the custom title bar's caption or maximize button sends non-client messages, which reach the
	// input handler as client ones so the application's UI follows the mouse there too. They're posted, so they come
	// through SDL's message loop, which calls this before dispatching each one; they become SDL events, so they're
	// handled in order with the client mouse events around them. False keeps the OS from having the message.
	bool SDLCALL NonClientMouseHook(void* stateData, MSG* msg)
	{
		const WindowNative::State& state = *static_cast<const WindowNative::State*>(stateData);
		if (msg->hwnd != state.Handle || !*state.HasCustomTitleBar)
		{
			return true;
		}

		const HWND windowHandle = state.Handle;
		const auto toClient = [windowHandle](LPARAM screenPosition)
		{
			POINT point = { GET_X_LPARAM(screenPosition), GET_Y_LPARAM(screenPosition) };
			ScreenToClient(windowHandle, &point);
			return point;
		};

		switch (msg->message)
		{
		case WM_NCMOUSEMOVE:
		{
			PushNonClientMouse(state, NonClientMouse_Move, toClient(msg->lParam));

			// For WM_NCMOUSELEAVE
			TRACKMOUSEEVENT track = { sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, windowHandle, 0 };
			TrackMouseEvent(&track);

			// The OS still sees it, e.g. to show Snap Layouts over the maximize button
			return true;
		}

		case WM_NCMOUSELEAVE:
		{
			// Wherever the mouse went: outside the client area if it left the window, so the UI stops showing it over the bar
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(windowHandle, &point);
			PushNonClientMouse(state, NonClientMouse_Move, point);
			return false;
		}

		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONDBLCLK:
		{
			// The maximize button is the application's: the OS would draw its own over it and track the click itself
			if (msg->wParam != HTMAXBUTTON)
			{
				return true;
			}
			PushNonClientMouse(state, NonClientMouse_Down, toClient(msg->lParam));
			return false;
		}

		case WM_NCLBUTTONUP:
		{
			// Also when it's let go elsewhere on the bar after being pressed on the maximize button (HandleEvent)
			PushNonClientMouse(state, NonClientMouse_Up, toClient(msg->lParam));
			return msg->wParam != HTMAXBUTTON;
		}

		default:
			return true;
		}
	}
}

namespace WindowNative
{
	StatePtr CreateState(SDL_Window* window, const Display::ScreenRect& workArea, int width, int height, const bool& hasCustomTitleBar, const TitleBarLayout& titleBarLayout)
	{
		StatePtr state(new State());
		state->SdlWindow = window;
		state->Handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
		state->HasCustomTitleBar = &hasCustomTitleBar;
		state->Layout = &titleBarLayout;
		state->NonClientMouseEvent = SDL_RegisterEvents(1);
		AssertNotNull(state->Handle);

		const HWND windowHandle = state->Handle;

		// The exe's icons, each at its own size, rather than SDL's one large one that Windows shrinks for the small
		SetClassLongPtr(windowHandle, GCLP_HICON, reinterpret_cast<LONG_PTR>(LoadAppIcon(SM_CXICON, SM_CYICON)));
		SetClassLongPtr(windowHandle, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(LoadAppIcon(SM_CXSMICON, SM_CYSMICON)));

		// Until Reveal. Unlike a hidden window, a cloaked one can be shown and maximized, and has its real size.
		const BOOL isCloaked = TRUE;
		DwmSetWindowAttribute(windowHandle, DWMWA_CLOAK, &isCloaked, sizeof(isCloaked));

		// Its outer size, frame included, and at the top-left of the monitor's work area
		state->Position = { workArea.Left, workArea.Top };
		SetWindowPos(windowHandle, nullptr, workArea.Left, workArea.Top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);

		// #NOTE(Josh) Gonna put this here for now instead of in Engine.cpp until it has a better home
		// A windowed app (the Editor) only has a console with -console
		if (HWND consoleWindow = GetConsoleWindow())
		{
			SetWindowPos(consoleWindow, 0, workArea.Left, workArea.Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
		}

		SetWindowSubclass(windowHandle, &WindowSubclassProc, k_subclassID, reinterpret_cast<DWORD_PTR>(state.get()));
		SDL_SetWindowsMessageHook(&NonClientMouseHook, state.get());

		return state;
	}

	void StateDeleter::operator()(State* state) const
	{
		SDL_SetWindowsMessageHook(nullptr, nullptr);
		RemoveWindowSubclass(state->Handle, &WindowSubclassProc, k_subclassID);
		delete state;
	}

	void Show(State& state)
	{
		// Still cloaked, so the user doesn't see it yet
		SDL_ShowWindow(state.SdlWindow);
	}

	void Reveal(State& state)
	{
		const BOOL isCloaked = FALSE;
		DwmSetWindowAttribute(state.Handle, DWMWA_CLOAK, &isCloaked, sizeof(isCloaked));
	}

	void SetSize(State& state, int width, int height)
	{
		::SetWindowPos(state.Handle, nullptr, state.Position.x, state.Position.y, width, height, SWP_NOZORDER);
	}

	void GetClientSize(State& state, int& outWidth, int& outHeight)
	{
		RECT windowRect;
		GetClientRect(state.Handle, &windowRect);
		outWidth = windowRect.right - windowRect.top;
		outHeight = windowRect.bottom - windowRect.top;
	}

	void OnCustomTitleBarChanged(State& state)
	{
		// Has the OS work out the client area again (WM_NCCALCSIZE)
		SetWindowPos(state.Handle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	int GetSystemTitleBarHeight(State& state)
	{
		const UINT dpi = GetDpiForWindow(state.Handle);
		return GetSystemMetricsForDpi(SM_CYCAPTION, dpi) + GetFrameThickness(state.Handle);
	}

	float GetDpiScale(State& state)
	{
		return static_cast<float>(GetDpiForWindow(state.Handle)) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
	}

	bool SetIcon(State& state, const Filepath& image)
	{
		const HWND windowHandle = state.Handle;
		std::unique_ptr<WindowIconWindows> icon = WindowIconWindows::FromImage(image, GetDpiForWindow(windowHandle));
		if (icon == nullptr)
		{
			return false;
		}

		SendMessage(windowHandle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon->GetLarge()));
		SendMessage(windowHandle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon->GetSmall()));
		// The class's too: the taskbar asks the window for its icon, and while the window is too busy to answer (e.g.
		// loading the first scene) it takes the class's instead, and keeps that
		SetClassLongPtr(windowHandle, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon->GetLarge()));
		SetClassLongPtr(windowHandle, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(icon->GetSmall()));
		// Only now, so the old icons are destroyed once the window has stopped using them
		state.Icon = std::move(icon);
		return true;
	}

	bool HandleEvent(State& state, const SDL_Event& event, InputHandler& inputHandler)
	{
		if (event.type != state.NonClientMouseEvent)
		{
			return false;
		}

		const Int32 x = static_cast<Int32>(reinterpret_cast<intptr_t>(event.user.data1));
		const Int32 y = static_cast<Int32>(reinterpret_cast<intptr_t>(event.user.data2));
		switch (event.user.code)
		{
		case NonClientMouse_Move:
			inputHandler.OnMouseMove(x, y);
			break;

		case NonClientMouse_Down:
			inputHandler.OnMouseDown(EMouseButton::MouseButton_Left, x, y);
			break;

		case NonClientMouse_Up:
			// Only if it was pressed on the maximize button, so it isn't left down
			if (inputHandler.GetProxyMouseState().CurMouseBtnStates[EMouseButton::MouseButton_Left])
			{
				inputHandler.OnMouseUp(EMouseButton::MouseButton_Left, x, y);
			}
			break;
		}
		return true;
	}
}
