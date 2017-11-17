#include <StdAfx.h>

#include <Windowing/Win32Window.h>

// RZE
#include <DebugUtils/Debug.h>

#include <Utils/Conversions.h>

#include <EngineCore/Input/InputHandler.h>
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
			WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME), // @note WS_POPUP = borderless. WS_OVERLAPPEDWINDOW default
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

void Win32Window::CompileInputMessages(InputHandler& inputHandler)
{
	MSG msg;
	if (PeekMessage(&msg, mOSWindowHandleData.windowHandle, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			const Int32 win32KeyCode = msg.wParam;
			const bool bIsRepeat = (msg.lParam & 0x40000000) != 0;

			inputHandler.OnKeyDown(win32KeyCode, bIsRepeat);
		}
		break;

		case WM_KEYUP:
		{
			const Int32 win32KeyCode = msg.wParam;
			inputHandler.OnKeyUp(win32KeyCode);
		}
		break;

		case WM_LBUTTONDOWN:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_LClick, 0, 0);
			//eventHandler.PostMouseEvent(mouseEvent);
		}
		break;

		case WM_LBUTTONUP:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_LRelease, 0, 0);
			//eventHandler.PostMouseEvent(mouseEvent);
		}
		break;

		case WM_MOUSEMOVE:
		{
			MouseEvent mouseEvent(EMouseEventType::Mouse_Move, static_cast<U32>(GET_X_LPARAM(msg.lParam)), static_cast<U32>(GET_Y_LPARAM(msg.lParam)));
			//eventHandler.PostMouseEvent(mouseEvent);
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
}

void Win32Window::CompileWindowMessages(EventHandler& eventHandler)
{
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
	else if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Resize)
	{
		WindowEvent windowEvent(EWindowEventType::Window_Resize);

		RECT windowRect;
		GetWindowRect(mOSWindowHandleData.windowHandle, &windowRect);
		int width = windowRect.right - windowRect.top;
		int height = windowRect.bottom - windowRect.top;

		windowEvent.mSizeX = width;
		windowEvent.mSizeY = height;

		eventHandler.PostWindowEvent(windowEvent);
	}
}

void Win32Window::InternalSetWindowPosition(const Vector2D& pos)
{
	mPos = pos;
	::SetWindowPos(mOSWindowHandleData.windowHandle, 0, static_cast<int>(mPos.X()), static_cast<int>(mPos.Y()), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Win32Window::InternalSetWindowSize(const Vector2D& size)
{
	mDimensions = size;
	::SetWindowPos(mOSWindowHandleData.windowHandle, nullptr, static_cast<int>(mPos.X()), static_cast<int>(mPos.Y()), static_cast<int>(size.X()), static_cast<int>(size.Y()), SWP_NOSIZE | SWP_NOZORDER);
}

void Win32Window::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> windowResizeCallback([this](const Event& evt)
	{
		mDimensions.SetX(evt.mWindowEvent.mSizeX);
		mDimensions.SetY(evt.mWindowEvent.mSizeY);
	});

	eventHandler.RegisterForEvent(EEventType::Window, windowResizeCallback);
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

	// #NOTE(Josh) This is fine for now, will make WM_SIZE it's own thing when necessary
	case WM_SIZE:
	case WM_SIZING:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Resize, wp, lp);
		return 0;
	}

	default:
	{
		return DefWindowProc(window, msg, wp, lp);
	}
	}
}