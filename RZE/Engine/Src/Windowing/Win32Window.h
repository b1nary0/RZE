#pragma once

// winapi
// @improvement does it suffice to have a pch here or refactor to avoid this in a .h?
#include <Windows.h>

// std lib
#include <memory>
#include <string>

// RZE
#include <Windowing/TitleBarLayout.h>
#include <Windowing/WindowMessageAdaptor.h>
#include <Utils/Math/Vector2D.h>

class EventHandler;
class Filepath;
class InputHandler;
class WindowIcon;

class Win32Window
{

private:

	typedef struct OSWindowHandleData
	{
		HWND windowHandle;
		HDC deviceContext;
		HGLRC renderContext;
		PIXELFORMATDESCRIPTOR pixelFormatDesc;
	} OSWindowHandleData;

public:

	struct WindowCreationParams
	{
		std::string		windowTitle;
		int				width{ -1 };
		int				height{ -1 };
		int				monitorIndex{ 0 };	// 0 = primary monitor
	};

	Win32Window(const WindowCreationParams& creationProtocol);
	~Win32Window();

	void SetCursorEnabled(bool enabled);
	void ResetCursorToCenter();

	void SetTitle(const std::string& title);
	// Shows image (e.g. a PNG) as the window's icon, in place of the executable's. False if it couldn't be read, which
	// leaves the icon as it was.
	bool SetIcon(const Filepath& image);

	const std::string& GetTitle() const;
	const Vector2D& GetDimensions() const;
	const Vector2D& GetClientSize() const;

	// Compile input messages from Win32 api
	void CompileInputMessages(InputHandler& inputHandler);

	// Compile all other window messages from Win32 api
	void CompileWindowMessages(EventHandler& eventHandler);

	void Show();
	// Puts it on screen. It's made cloaked: shown, maximized and so on as usual, but the user doesn't see it until this,
	// so it can be readied (sized, drawn) first.
	void Reveal();
	// Makes it the foreground window, the one the user is working in
	void Focus();

	// The window the user is working in, i.e. the foreground window
	bool IsFocused() const;
	bool IsMinimised() const;
	// Whether a mouse button is down right now, as the OS sees it. Unlike the input handler's state, it can't get stuck
	// down when the button is released outside the window.
	bool IsMouseButtonDown() const;

	const OSWindowHandleData& GetOSWindowHandleData() const;

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

protected:

private:

	static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	void Create(const WindowCreationParams& creationProtocol);

	// With a custom title bar: the client area takes the OS caption's place (WM_NCCALCSIZE), and the parts of it
	// that are the title bar are reported as caption, maximize button or resize border (WM_NCHITTEST)
	LRESULT CalculateClientArea(WPARAM wParam, LPARAM lParam);
	LRESULT HitTest(WPARAM wParam, LPARAM lParam);
	// The mouse over the custom title bar's caption or maximize button sends non-client messages, which reach the
	// input handler as client ones so the application's UI follows the mouse there too. False lets the OS have the
	// message as usual.
	bool CompileNonClientMouseMessage(const MSG& msg, InputHandler& inputHandler);
	// The invisible resize border's thickness, at the window's DPI
	int GetFrameThickness() const;

	void ProcessWinProcMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler);

	void InternalSetWindowPosition(const Vector2D& pos);

	void InternalSetWindowSize(const Vector2D& size);

	// @note right now (and maybe forever) we need to query winapi to update our client size every time
	// a change in window happens
	void QueryClientSize();


	OSWindowHandleData m_osWindowHandleData;

	std::string m_title;
	Vector2D m_dimensions;
	Vector2D m_clientSize;
	Vector2D m_pos;

	bool bCursorEnabled;

	bool m_hasCustomTitleBar{ false };
	TitleBarLayout m_titleBarLayout;

	// From SetIcon; null while the window shows the executable's icon
	std::unique_ptr<WindowIcon> m_icon;
};
