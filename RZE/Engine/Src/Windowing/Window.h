#pragma once

// std lib
#include <memory>
#include <string>
#include <vector>

// RZE
#include <Windowing/TitleBarLayout.h>
#include <Windowing/WindowNativeState.h>
#include <Rendering/Driver/GraphicsAPI.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

class EventHandler;
class Filepath;
class InputHandler;

struct SDL_Window;

// The application's window. The same class on every platform: SDL underneath, with the few things SDL doesn't do the
// same everywhere (WindowNative.h) done natively.
class Window
{
public:

	struct WindowCreationParams
	{
		std::string		windowTitle;
		int				width{ -1 };
		int				height{ -1 };
		int				monitorIndex{ 0 };	// 0 = primary monitor
		// What the renderer draws in it with, since SDL makes a window for OpenGL differently
		Rendering::EGraphicsAPI	graphicsAPI{ Rendering::EGraphicsAPI::DX11 };
	};

	Window(const WindowCreationParams& creationProtocol);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void SetTitle(const std::string& title);
	// Shows image (e.g. a PNG) as the window's icon, in place of the executable's. False if it couldn't be read, which
	// leaves the icon as it was.
	bool SetIcon(const Filepath& image);

	const std::string& GetTitle() const;
	const Vector2D& GetDimensions() const;
	const Vector2D& GetClientSize() const;

	// Compile input messages from the OS (through SDL)
	void CompileInputMessages(InputHandler& inputHandler);

	// Compile all other window messages from the OS, gathered by CompileInputMessages
	void CompileWindowMessages(EventHandler& eventHandler);

	void Show();
	// Puts it on screen. It's made hidden from the user: shown, maximized and so on as usual, but the user doesn't see
	// it until this, so it can be readied (sized, drawn) first.
	void Reveal();
	// Makes it the foreground window, the one the user is working in
	void Focus();

	// The window the user is working in, i.e. the foreground window
	bool IsFocused() const;
	bool IsMinimised() const;
	// Whether a mouse button is down right now, as the OS sees it. Unlike the input handler's state, it can't get stuck
	// down when the button is released outside the window.
	bool IsMouseButtonDown() const;

	// The SDL_Window, for code that needs the OS's window (the renderer, Display::GetRefreshRate)
	void* GetNativeHandle() const;

	void RegisterEvents(EventHandler& eventHandler);

	void SetWindowSize(const Vector2D& newSize);

	void Maximize();
	void Minimize();
	// Back to its size before it was maximized or minimized
	void Restore();
	bool IsMaximised() const;
	// As if its close button was pressed: the application's OnCloseRequested decides
	void RequestClose();

	// Leaves out the OS's title bar, for an application that draws its own across the top of the client area. The
	// window still moves, snaps, resizes and maximizes as usual, from what SetTitleBarLayout says the bar is.
	void SetCustomTitleBar(bool isCustom);
	// Where the application's own title bar is, once a UI frame; the OS asks about it as the mouse moves
	void SetTitleBarLayout(TitleBarLayout&& layout);
	// The height of the OS's title bar at the window's DPI, for a custom one to match
	int GetSystemTitleBarHeight() const;
	// The window's DPI over the standard 96, e.g. 1.25
	float GetDpiScale() const;

private:
	enum class EPendingEvent
	{
		Destroy,
		CloseRequested,
		Resize
	};

	void QueryClientSize();
	// Once per pump: many resizes in a row (e.g. while the edge is dragged) are told as one
	void AddPendingEvent(EPendingEvent pendingEvent);

	SDL_Window* m_window = nullptr;
	WindowNative::StatePtr m_native;
	bool m_isVideoAcquired = false;

	std::string m_title;
	Vector2D m_dimensions;
	Vector2D m_clientSize;

	bool m_hasCustomTitleBar{ false };
	TitleBarLayout m_titleBarLayout;

	// The window's events from the last CompileInputMessages, for CompileWindowMessages
	std::vector<EPendingEvent> m_pendingEvents;
};
