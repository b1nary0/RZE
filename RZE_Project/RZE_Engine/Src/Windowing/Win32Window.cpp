#include <StdAfx.h>

#include <Windowing/Win32Window.h>

// RZE
#include <DebugUtils/Debug.h>

#include <Utils/Conversions.h>

#include <Events/EventHandler.h>

#include <Windowing/WindowMessageAdaptor.h>
#include <Windowing/WinKeyCodes.h>

LRESULT CALLBACK WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp);

// Used to link WinProc messages with the window without having to static other class instances etc
static WindowMessageAdaptor sWindowMessageAdaptor;

Win32Window::Win32Window(const WindowCreationParams& creationProtocol)
{
	mTitle = creationProtocol.windowTitle;
	mDimensions = Vector2D(static_cast<float>(creationProtocol.width), static_cast<float>(creationProtocol.height));

	Create(creationProtocol);
}

void Win32Window::SetCursorEnabled(bool enabled)
{
	bCursorEnabled = enabled;
}

void Win32Window::ResetCursorToCenter()
{
	if (!bCursorEnabled)
	{
		RECT rcClip;
		GetClientRect(mOSWindowHandleData.windowHandle, &rcClip);
		MapWindowPoints(mOSWindowHandleData.windowHandle, GetParent(mOSWindowHandleData.windowHandle), (LPPOINT)(&rcClip), 1);
		ClipCursor(&rcClip);
		SetCursorPos(rcClip.left + static_cast<int>(mDimensions.X() / 2), rcClip.top + static_cast<int>(mDimensions.Y() / 2));
	}
}

const std::string& Win32Window::GetTitle() const
{
	return mTitle;
}

const Vector2D& Win32Window::GetDimensions() const
{
	return mDimensions;
}

void Win32Window::Create(const WindowCreationParams& creationProtocol)
{
	// Interface with WinAPI and create a window -- store useful information.
	LOG_CONSOLE_ANNOUNCE("[ Creating Win32Window ] Title : %s", mTitle.c_str());

	const std::wstring wStrTitle = Conversions::StringToWString(mTitle);

	BYTE CursorMaskAND[] = { 0xFF };
	BYTE CursorMaskXOR[] = { 0x00 };

	// @NOTE what do the properties that are set as 0 do?
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW; // @NOTE see https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176(v=vs.85).aspx
	wc.lpfnWndProc = WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadIcon(0, IDI_APPLICATION); // @TODO fill this with proper customizable icon
	wc.hCursor = /*CreateCursor(NULL, 0, 0, 1, 1, CursorMaskAND, CursorMaskXOR);*/ LoadCursor(NULL, IDC_ARROW);  // @TODO fill this with proper customizable cursor
	wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1); // @NOTE what is this?
	wc.lpszMenuName = 0;
	wc.lpszClassName = wStrTitle.c_str(); // @NOTE does this represent what you think it represents?
	wc.hIconSm = LoadIcon(0, IDI_APPLICATION); // @TODO fill this with proper customizable icon

	if (RegisterClassEx(&wc))
	{
		mOSWindowHandleData.windowHandle = CreateWindowEx(0,
			wStrTitle.c_str(),
			wStrTitle.c_str(),
			WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX), // @note WS_POPUP = borderless. WS_OVERLAPPEDWINDOW default
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<int>(mDimensions.X()),
			static_cast<int>(mDimensions.Y()),
			0,
			0,
			GetModuleHandle(0),
			0);

		mOSWindowHandleData.pixelFormatDesc =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			32,                        //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                        //Number of bits for the depthbuffer
			8,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		//@todo this needs to be abstracted out of the window somehow 

		mOSWindowHandleData.deviceContext = GetDC(mOSWindowHandleData.windowHandle);
		if (!mOSWindowHandleData.deviceContext)
		{
			AssertFalse();
		}

		int pixelFormat = ChoosePixelFormat(mOSWindowHandleData.deviceContext, &mOSWindowHandleData.pixelFormatDesc);
		if (!pixelFormat)
		{
			AssertFalse();
		}

		if (!SetPixelFormat(mOSWindowHandleData.deviceContext, pixelFormat, &mOSWindowHandleData.pixelFormatDesc))
		{
			AssertFalse();
		}

		mOSWindowHandleData.renderContext = wglCreateContext(mOSWindowHandleData.deviceContext);
		if (!mOSWindowHandleData.renderContext)
		{
			AssertFalse();
		}

		if (!wglMakeCurrent(mOSWindowHandleData.deviceContext, mOSWindowHandleData.renderContext))
		{
			AssertFalse();
		}

		InternalSetWindowPosition(Vector2D(0, 0));
	}
}

void Win32Window::CompileMessages(EventHandler& eventHandler)
{
	MSG msg;
	if (PeekMessage(&msg, mOSWindowHandleData.windowHandle, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{

		case WM_KEYDOWN:
		{
			KeyEvent keyEvent(EKeyEventType::Key_Pressed, static_cast<U8>(msg.wParam));
			eventHandler.PostKeyEvent(keyEvent);
		}
		break;

		case WM_KEYUP:
		{
			KeyEvent keyEvent(EKeyEventType::Key_Released, static_cast<U8>(msg.wParam));
			eventHandler.PostKeyEvent(keyEvent);
		}
		break;

		case WM_LBUTTONDOWN:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_LClick, 0, 0);
			eventHandler.PostMouseEvent(mouseEvent);
		}
		break;

		case WM_LBUTTONUP:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_LRelease, 0, 0);
			eventHandler.PostMouseEvent(mouseEvent);
		}
		break;

		case WM_MOUSEMOVE:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_Move, static_cast<U32>(GET_X_LPARAM(msg.lParam)), static_cast<U32>(GET_Y_LPARAM(msg.lParam)));
			eventHandler.PostMouseEvent(mouseEvent);
		}
		break;

		case WM_SETCURSOR:
		{
			if (!bCursorEnabled)
			{
				SetCursor(NULL);
			}
		}
		break;

		default:
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		break;

		}
	}

	// @note may not need this if we don't need to process multiple messages from window per frame
	while (sWindowMessageAdaptor.HasMessage())
	{
		ProcessWinProcMessage(sWindowMessageAdaptor.GetNextMessage(), eventHandler);
	}
}

void Win32Window::Show()
{
	if (mOSWindowHandleData.windowHandle)
	{
		ShowWindow(mOSWindowHandleData.windowHandle, SW_SHOW); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default
	}
}

void Win32Window::BufferSwap() const
{
	SwapBuffers(mOSWindowHandleData.deviceContext);
}

const Win32Window::OSWindowHandleData& Win32Window::GetOSWindowHandleData() const
{
	return mOSWindowHandleData;
}

void Win32Window::ProcessWinProcMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler)
{
	if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Destroy)
	{
		WindowEvent windowEvent(EWindowEventType::Window_Destroy);
		eventHandler.PostWindowEvent(windowEvent);
	}
}

void Win32Window::InternalSetWindowPosition(const Vector2D& pos)
{
	SetWindowPos(mOSWindowHandleData.windowHandle, 0, static_cast<int>(pos.X()), static_cast<int>(pos.Y()), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
	// @unfinished - check for values of msg possibly needed
	switch (msg)
	{
	case WM_CREATE:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Create, wp, lp);
		return 0;
	}

	case WM_DESTROY:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Destroy, wp, lp);
		return 0;
	}

	default:
	{
		return DefWindowProc(window, msg, wp, lp);
	}
	}
}
