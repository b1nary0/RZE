#include "Win32Window.h"

// WinAPI
#include <Windows.h>

// std lib
#include <locale.h>
#include <codecvt>

// RZE
#include "../Utils/Conversions.h"

long __stdcall WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp);

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
	return mTitle;
}

const Vector2D& Win32Window::GetDimensions() const
{
	return mDimensions;
}

void Win32Window::Create(const WindowCreationProtocol& creationProtocol)
{
	// Interface with WinAPI and create a window -- store useful information.
	printf("[ Creating Win32Window ] Title : %s | ID: %i", mTitle.c_str(), mWindowID);

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
			ShowWindow(mOSWindowHandleData.mWindowHandle, SW_SHOWDEFAULT); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default

			MSG msg;
			while (GetMessage(&msg, 0, 0, 0)) DispatchMessage(&msg);
		}
	}
}

long __stdcall WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
	// @unfinished - check for values of msg possibly needed
	switch (msg)
	{
	case WM_ACTIVATE:
		return 0;
		break;

	case WM_SYSCOMMAND:
		return 0;
		break;

	case WM_CLOSE:
		// @implementation - will need to pass a message through the engine to shut down properly here
		PostQuitMessage(0);
		return 0;
		break;

	case WM_KEYDOWN:
		printf("Key down! -> %c\n", static_cast<char>(wp));
		return 0;
		break;

	case WM_KEYUP:
		printf("Key up! -> %c\n", static_cast<char>(wp));
		return 0;
		break;

	case WM_SIZE:
		return 0;
		break;

	default:
		return DefWindowProc(window, msg, wp, lp);
	}
}
