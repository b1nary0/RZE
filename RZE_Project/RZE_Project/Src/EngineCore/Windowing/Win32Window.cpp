#include "StdAfx.h"

#include "Win32Window.h"

// RZE
#include "WindowMessageAdaptor.h"
#include "EngineCore/Events/EventHandler.h"
#include "EngineCore/Utils/Conversions.h"

long __stdcall WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp);

// Used to link WinProc messages with the window without having to static other class instances etc
static WindowMessageAdaptor sWindowMessageAdaptor;

struct OSWindowHandleData
{
	HWND mWindowHandle;
};

Win32Window::Win32Window(const WindowCreationProtocol& creationProtocol)
{
	mTitle = creationProtocol.WindowTitle;
	mDimensions = Vector2D(static_cast<float>(creationProtocol.Width), static_cast<float>(creationProtocol.Height));
	mWindowID = creationProtocol.WindowID;

	Create(creationProtocol);
}

const std::string& Win32Window::GetTitle() const
{
	assert(!mTitle.empty() && "Returning empty window title!");
	return mTitle;
}

const Vector2D& Win32Window::GetDimensions() const
{
	return mDimensions;
}

void Win32Window::Create(const WindowCreationProtocol& creationProtocol)
{
	// Interface with WinAPI and create a window -- store useful information.
	printf("[ Creating Win32Window ] Title : %s | ID: %i\n", mTitle.c_str(), mWindowID);

	const std::wstring wStrTitle = Conversions::StringToWString(mTitle);

	// @NOTE what do the properties that are set as 0 do?
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW; // @NOTE see https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176(v=vs.85).aspx
	wc.lpfnWndProc = WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadIcon(0, IDI_APPLICATION); // @TODO fill this with proper customizable icon
	wc.hCursor = LoadCursor(0, IDC_ARROW); // @TODO fill this with proper customizable cursor
	wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1); // @NOTE what is this?
	wc.lpszMenuName = 0;
	wc.lpszClassName = wStrTitle.c_str(); // @NOTE does this represent what you think it represents?
	wc.hIconSm = LoadIcon(0, IDI_APPLICATION); // @TODO fill this with proper customizable icon

	if (RegisterClassEx(&wc))
	{
		mOSWindowHandleData.mWindowHandle = CreateWindowEx(0,
			wStrTitle.c_str(),
			wStrTitle.c_str(),
			WS_OVERLAPPEDWINDOW, // @note WS_POPUP = borderless. WS_OVERLAPPEDWINDOW default
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<int>(mDimensions.X()),
			static_cast<int>(mDimensions.Y()),
			0,
			0,
			GetModuleHandle(0),
			0);

		if (mOSWindowHandleData.mWindowHandle)
		{
			ShowWindow(mOSWindowHandleData.mWindowHandle, SW_NORMAL); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default
		}
	}
}

void Win32Window::ProcessWindowMessages(EventHandler& eventHandler)
{
	MSG msg;
	if (PeekMessage(&msg, mOSWindowHandleData.mWindowHandle, 0, 0, PM_NOREMOVE))
	{
		GetMessage(&msg, mOSWindowHandleData.mWindowHandle, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// @note may not need this if we don't need to process multiple messages from window per frame
	while (sWindowMessageAdaptor.HasMessage())
	{
		ProcessWindowMessage(sWindowMessageAdaptor.GetNextMessage(), eventHandler);
	}
}

void Win32Window::ProcessWindowMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler)
{
	if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_KeyDown)
	{
	}
	else if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Destroy)
	{
		printf("Window set to destroy.\n");
		eventHandler.RegisterWindowEvent(WindowEvent(0, 1), true);
	}
}

long __stdcall WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
	// @unfinished - check for values of msg possibly needed
	switch (msg)
	{
	case WM_KEYDOWN:
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_KeyDown, wp, lp);
		return 0;

	case WM_KEYUP:
		return 0;

	case WM_SIZE:
		return 0;
		
	case WM_DESTROY:
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Destroy, wp, lp);
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(window, msg, wp, lp);
	}
}
