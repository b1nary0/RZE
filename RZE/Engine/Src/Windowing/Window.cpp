#include <Windowing/Window.h>

// RZE
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Input/KeyCodes.h>
#include <Events/EventHandler.h>

#include <Windowing/Display.h>
#include <Windowing/SDLVideo.h>
#include <Windowing/WindowNative.h>

#include <Utils/DebugUtils/Debug.h>

#include <SDL3/SDL.h>

// std lib
#include <algorithm>
#include <bitset>
#include <cmath>

namespace
{
	// The engine's key code for an SDL key, 0 for one it has none for. SDL's keycodes follow the keyboard layout, as
	// Windows' virtual keys do (KeyCodes.h), and with SDL's default options (French number row, Latin letters) they
	// match them on other layouts too. Left and right Shift, Control and Alt are told apart by neither.
	Int32 ToKeyCode(SDL_Keycode key, SDL_Keymod modifiers)
	{
		if (key >= SDLK_A && key <= SDLK_Z)
		{
			return KeyCode::Key_A + static_cast<Int32>(key - SDLK_A);
		}
		if (key >= SDLK_0 && key <= SDLK_9)
		{
			return KeyCode::Key_0 + static_cast<Int32>(key - SDLK_0);
		}
		if (key >= SDLK_F1 && key <= SDLK_F12)
		{
			return KeyCode::F1 + static_cast<Int32>(key - SDLK_F1);
		}
		if (key >= SDLK_F13 && key <= SDLK_F24)
		{
			return KeyCode::F13 + static_cast<Int32>(key - SDLK_F13);
		}

		// Windows gives the keypad's digits only with Num Lock on, and the keys printed under them otherwise
		const bool isNumLockOn = (modifiers & SDL_KMOD_NUM) != 0;
		switch (key)
		{
		case SDLK_KP_0: return isNumLockOn ? KeyCode::Numpad0 : KeyCode::Insert;
		case SDLK_KP_1: return isNumLockOn ? KeyCode::Numpad1 : KeyCode::End;
		case SDLK_KP_2: return isNumLockOn ? KeyCode::Numpad2 : KeyCode::Down;
		case SDLK_KP_3: return isNumLockOn ? KeyCode::Numpad3 : KeyCode::Next;
		case SDLK_KP_4: return isNumLockOn ? KeyCode::Numpad4 : KeyCode::LEFT;
		case SDLK_KP_5: return isNumLockOn ? KeyCode::Numpad5 : KeyCode::Clear;
		case SDLK_KP_6: return isNumLockOn ? KeyCode::Numpad6 : KeyCode::Right;
		case SDLK_KP_7: return isNumLockOn ? KeyCode::Numpad7 : KeyCode::Home;
		case SDLK_KP_8: return isNumLockOn ? KeyCode::Numpad8 : KeyCode::Up;
		case SDLK_KP_9: return isNumLockOn ? KeyCode::Numpad9 : KeyCode::Prior;
		case SDLK_KP_PERIOD: return isNumLockOn ? KeyCode::Decimal : KeyCode::Delete;
		case SDLK_KP_DIVIDE: return KeyCode::Divide;
		case SDLK_KP_MULTIPLY: return KeyCode::Multiply;
		case SDLK_KP_MINUS: return KeyCode::Subtract;
		case SDLK_KP_PLUS: return KeyCode::Add;
		case SDLK_KP_ENTER: return KeyCode::Return;

		case SDLK_RETURN: return KeyCode::Return;
		case SDLK_ESCAPE: return KeyCode::Escape;
		case SDLK_BACKSPACE: return KeyCode::Backspace;
		case SDLK_TAB: return KeyCode::Tab;
		case SDLK_SPACE: return KeyCode::Space;

		case SDLK_INSERT: return KeyCode::Insert;
		case SDLK_DELETE: return KeyCode::Delete;
		case SDLK_HOME: return KeyCode::Home;
		case SDLK_END: return KeyCode::End;
		case SDLK_PAGEUP: return KeyCode::Prior;
		case SDLK_PAGEDOWN: return KeyCode::Next;
		case SDLK_LEFT: return KeyCode::LEFT;
		case SDLK_RIGHT: return KeyCode::Right;
		case SDLK_UP: return KeyCode::Up;
		case SDLK_DOWN: return KeyCode::Down;
		case SDLK_CLEAR: return KeyCode::Clear;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return KeyCode::Shift;
		case SDLK_LCTRL:
		case SDLK_RCTRL: return KeyCode::Control;
		case SDLK_LALT:
		case SDLK_RALT: return KeyCode::Menu;
		case SDLK_LGUI: return KeyCode::LWin;
		case SDLK_RGUI: return KeyCode::RWin;
		case SDLK_APPLICATION: return KeyCode::Apps;

		case SDLK_CAPSLOCK: return KeyCode::Capital;
		case SDLK_NUMLOCKCLEAR: return KeyCode::Numlock;
		case SDLK_SCROLLLOCK: return KeyCode::Scroll;
		case SDLK_PRINTSCREEN: return KeyCode::Snapshot;
		case SDLK_PAUSE: return KeyCode::Pause;
		case SDLK_SLEEP: return KeyCode::Sleep;

		// The US layout's punctuation keys, by the character they type, as Windows names them
		case SDLK_SEMICOLON: return KeyCode::OEM_1;
		case SDLK_EQUALS: return KeyCode::OEM_Plus;
		case SDLK_COMMA: return KeyCode::OEM_Comma;
		case SDLK_MINUS: return KeyCode::OEM_Minus;
		case SDLK_PERIOD: return KeyCode::OEM_Period;
		case SDLK_SLASH: return KeyCode::OEM_2;
		case SDLK_GRAVE: return KeyCode::OEM_3;
		case SDLK_LEFTBRACKET: return KeyCode::OEM_4;
		case SDLK_BACKSLASH: return KeyCode::OEM_5;
		case SDLK_RIGHTBRACKET: return KeyCode::OEM_6;
		case SDLK_APOSTROPHE: return KeyCode::OEM_7;

		case SDLK_MUTE: return KeyCode::Volume_Mute;
		case SDLK_VOLUMEDOWN: return KeyCode::Volume_Down;
		case SDLK_VOLUMEUP: return KeyCode::Volume_Up;
		case SDLK_MEDIA_NEXT_TRACK: return KeyCode::Media_Next_Track;
		case SDLK_MEDIA_PREVIOUS_TRACK: return KeyCode::Media_Prev_Track;
		case SDLK_MEDIA_STOP: return KeyCode::Media_Stop;
		case SDLK_MEDIA_PLAY_PAUSE: return KeyCode::Media_Play_Pause;
		case SDLK_AC_BACK: return KeyCode::Browser_Back;
		case SDLK_AC_FORWARD: return KeyCode::Browser_Forward;
		case SDLK_AC_REFRESH: return KeyCode::Browser_Refresh;
		case SDLK_AC_STOP: return KeyCode::Browser_Stop;
		case SDLK_AC_SEARCH: return KeyCode::Browser_Search;
		case SDLK_AC_BOOKMARKS: return KeyCode::Browser_Favorites;
		case SDLK_AC_HOME: return KeyCode::Browser_Home;

		default: return 0;
		}
	}

	EMouseButton::T ToMouseButton(Uint8 button)
	{
		switch (button)
		{
		case SDL_BUTTON_RIGHT: return EMouseButton::MouseButton_Right;
		case SDL_BUTTON_MIDDLE: return EMouseButton::MouseButton_Middle;
		case SDL_BUTTON_X1: return EMouseButton::MouseButton_Back;
		case SDL_BUTTON_X2: return EMouseButton::MouseButton_Forward;
		case SDL_BUTTON_LEFT:
		default: return EMouseButton::MouseButton_Left;
		}
	}
}

Window::Window(const WindowCreationParams& creationProtocol)
{
	m_title = creationProtocol.windowTitle;
	m_dimensions = Vector2D(static_cast<float>(creationProtocol.width), static_cast<float>(creationProtocol.height));

	RZE_LOG_ARGS("Creating window [%s]", m_title.c_str());

	m_isVideoAcquired = SDLVideo::Acquire();
	AssertExpr(m_isVideoAcquired);

	Display::MonitorInfo monitorInfo;
	if (!Display::GetMonitor(creationProtocol.monitorIndex, monitorInfo))
	{
		RZE_LOG_WARNING_ARGS("Launch monitor %d out of range (%d monitors). Using primary.", creationProtocol.monitorIndex, static_cast<int>(Display::EnumerateMonitors().size()));
		Display::GetMonitor(0, monitorInfo);
	}

	// Hidden until it's shown (Show, Reveal), and sizable like any normal window. OpenGL's needs a pixel format that
	// allows it, which SDL only gives a window made for it.
	SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
	if (creationProtocol.graphicsAPI == Rendering::EGraphicsAPI::OpenGL)
	{
		flags |= SDL_WINDOW_OPENGL;
	}
	m_window = SDL_CreateWindow(m_title.c_str(), creationProtocol.width, creationProtocol.height, flags);
	if (m_window == nullptr)
	{
		RZE_LOG_ERROR("<Failure creating the window> " << SDL_GetError());
		AssertFalse();
		return;
	}

	m_native = WindowNative::CreateState(m_window, monitorInfo.WorkArea, creationProtocol.width, creationProtocol.height, m_hasCustomTitleBar, m_titleBarLayout);

	QueryClientSize();
}

Window::~Window()
{
	// Before the window it's for, not after this body as a member would be
	m_native.reset();
	if (m_window != nullptr)
	{
		SDL_DestroyWindow(m_window);
	}
	if (m_isVideoAcquired)
	{
		SDLVideo::Release();
	}
}

void Window::SetTitle(const std::string& title)
{
	m_title = title;
	SDL_SetWindowTitle(m_window, title.c_str());
}

bool Window::SetIcon(const Filepath& image)
{
	return WindowNative::SetIcon(*m_native, image);
}

const std::string& Window::GetTitle() const
{
	return m_title;
}

const Vector2D& Window::GetDimensions() const
{
	return m_dimensions;
}

const Vector2D& Window::GetClientSize() const
{
	return m_clientSize;
}

void Window::CompileInputMessages(InputHandler& inputHandler)
{
	// The wheel only reports deltas, so clear it each frame. Check the proxy state: the raised state is
	// only refreshed by RaiseEvents(), so it stays stale while the application is stealing input.
	if (inputHandler.GetProxyMouseState().CurWheelVal != 0)
	{
		inputHandler.ClearMouseWheel();
	}

	// The engine sees keys as down or up each frame, so a key pressed and released in one pump would never be seen
	// down. That happens with fast typing at a low frame rate, or when the OS hands over a press and its release
	// together, as KDE does for keys that type text while text input is on. Their releases are told now, a frame on.
	for (const Int32 keyCode : m_deferredKeyUps)
	{
		inputHandler.OnKeyUp(keyCode);
	}
	m_deferredKeyUps.clear();
	std::bitset<256> pressedThisPump;

	const SDL_WindowID windowID = SDL_GetWindowID(m_window);

	// Every queued event, not one per frame: with frames capped, one per frame lets input queue up and lag
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (WindowNative::HandleEvent(*m_native, event, inputHandler))
		{
			continue;
		}

		switch (event.type)
		{
		case SDL_EVENT_TEXT_INPUT:
		{
			if (event.text.windowID == windowID && ImGui::GetIO().WantCaptureKeyboard)
			{
				ImGui::GetIO().AddInputCharactersUTF8(event.text.text);
			}
		}
		break;

		case SDL_EVENT_KEY_DOWN:
		{
			const Int32 keyCode = ToKeyCode(event.key.key, event.key.mod);
			if (event.key.windowID == windowID && keyCode != 0)
			{
				inputHandler.OnKeyDown(keyCode, event.key.repeat);
				pressedThisPump.set(static_cast<size_t>(keyCode));
			}
		}
		break;

		case SDL_EVENT_KEY_UP:
		{
			const Int32 keyCode = ToKeyCode(event.key.key, event.key.mod);
			if (event.key.windowID == windowID && keyCode != 0)
			{
				if (pressedThisPump.test(static_cast<size_t>(keyCode)))
				{
					m_deferredKeyUps.push_back(keyCode);
				}
				else
				{
					inputHandler.OnKeyUp(keyCode);
				}
			}
		}
		break;

		case SDL_EVENT_MOUSE_MOTION:
		{
			if (event.motion.windowID == windowID)
			{
				inputHandler.OnMouseMove(static_cast<Int32>(event.motion.x), static_cast<Int32>(event.motion.y));
			}
		}
		break;

		// While a button is down SDL keeps the mouse, so the release still reaches the window when it happens outside
		// it (e.g. a drag that ends past its edge)
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			if (event.button.windowID == windowID)
			{
				inputHandler.OnMouseDown(ToMouseButton(event.button.button), static_cast<Int32>(event.button.x), static_cast<Int32>(event.button.y));
			}
		}
		break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			if (event.button.windowID == windowID)
			{
				inputHandler.OnMouseUp(ToMouseButton(event.button.button), static_cast<Int32>(event.button.x), static_cast<Int32>(event.button.y));
			}
		}
		break;

		case SDL_EVENT_MOUSE_WHEEL:
		{
			if (event.wheel.windowID == windowID)
			{
				// In notches; the engine counts as Windows does, k_mouseWheelNotchDelta to a notch, away from the user
				// positive
				const float notches = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
				inputHandler.OnMouseWheel(static_cast<Int32>(std::lround(notches * k_mouseWheelNotchDelta)));
			}
		}
		break;

		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		{
			// SDL doesn't close it: RZE_Application::OnCloseRequested decides
			if (event.window.windowID == windowID)
			{
				AddPendingEvent(EPendingEvent::CloseRequested);
			}
		}
		break;

		case SDL_EVENT_WINDOW_DESTROYED:
		{
			if (event.window.windowID == windowID)
			{
				AddPendingEvent(EPendingEvent::Destroy);
			}
		}
		break;

		// Also on moving to another monitor, which may have another refresh rate
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
		{
			if (event.window.windowID == windowID)
			{
				AddPendingEvent(EPendingEvent::Resize);
			}
		}
		break;

		default:
			break;
		}
	}
}

void Window::CompileWindowMessages(EventHandler& eventHandler)
{
	for (const EPendingEvent pendingEvent : m_pendingEvents)
	{
		if (pendingEvent == EPendingEvent::Destroy)
		{
			WindowEvent windowEvent(EWindowEventType::Window_Destroy);
			eventHandler.PostWindowEvent(windowEvent);
		}
		else if (pendingEvent == EPendingEvent::CloseRequested)
		{
			WindowEvent windowEvent(EWindowEventType::Window_CloseRequested);
			eventHandler.PostWindowEvent(windowEvent);
		}
		else if (pendingEvent == EPendingEvent::Resize)
		{
			WindowEvent windowEvent(EWindowEventType::Window_Resize);

			QueryClientSize();

			const Vector2D& clientSize = GetClientSize();
			// @todo this is a great example of why making a Vec2Di class is good. Or just impose usage on APIs to be in the w/h values
			const int width = static_cast<int>(clientSize.X());
			const int height = static_cast<int>(clientSize.Y());
			if (width > 0 && height > 0)
			{
				windowEvent.mSizeX = width;
				windowEvent.mSizeY = height;
				eventHandler.PostWindowEvent(windowEvent);
			}
			else
			{
				RZE_LOG_WARNING("Setting 0 dimension window size. Could be minimized. Is this expected?");
			}
		}
	}
	m_pendingEvents.clear();
}

void Window::AddPendingEvent(EPendingEvent pendingEvent)
{
	if (std::find(m_pendingEvents.begin(), m_pendingEvents.end(), pendingEvent) == m_pendingEvents.end())
	{
		m_pendingEvents.push_back(pendingEvent);
	}
}

void Window::Reveal()
{
	WindowNative::Reveal(*m_native);
}

void Window::Focus()
{
	SDL_RaiseWindow(m_window);
}

void Window::Show()
{
	WindowNative::Show(*m_native);
}

bool Window::IsFocused() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool Window::IsMinimised() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) != 0;
}

bool Window::IsMouseButtonDown() const
{
	// Any of them
	return SDL_GetGlobalMouseState(nullptr, nullptr) != 0;
}

void* Window::GetNativeHandle() const
{
	return m_window;
}

void Window::QueryClientSize()
{
	int width = 0;
	int height = 0;
	WindowNative::GetClientSize(*m_native, width, height);

	m_clientSize.SetXY(static_cast<float>(width), static_cast<float>(height));
}

void Window::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> windowResizeCallback([this](const Event& evt)
	{
		m_dimensions.SetX(evt.mWindowEvent.mSizeX);
		m_dimensions.SetY(evt.mWindowEvent.mSizeY);
	});

	eventHandler.RegisterForEvent(EEventType::Window, windowResizeCallback);
}

void Window::SetWindowSize(const Vector2D& newSize)
{
	m_dimensions = newSize;
	WindowNative::SetSize(*m_native, static_cast<int>(newSize.X()), static_cast<int>(newSize.Y()));
}

void Window::Maximize()
{
	SDL_MaximizeWindow(m_window);

	QueryClientSize();
}

void Window::Minimize()
{
	SDL_MinimizeWindow(m_window);
}

void Window::Restore()
{
	SDL_RestoreWindow(m_window);

	QueryClientSize();
}

bool Window::IsMaximised() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) != 0;
}

void Window::RequestClose()
{
	// Handled with the next events, as if its close button was pressed
	SDL_Event event = {};
	event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
	event.window.windowID = SDL_GetWindowID(m_window);
	SDL_PushEvent(&event);
}

void Window::SetCustomTitleBar(bool isCustom)
{
	m_hasCustomTitleBar = isCustom;

	WindowNative::OnCustomTitleBarChanged(*m_native);
	QueryClientSize();
}

void Window::SetTitleBarLayout(TitleBarLayout&& layout)
{
	m_titleBarLayout = std::move(layout);
}

int Window::GetSystemTitleBarHeight() const
{
	return WindowNative::GetSystemTitleBarHeight(*m_native);
}

float Window::GetDpiScale() const
{
	return WindowNative::GetDpiScale(*m_native);
}
