#include <StdAfx.h>

#include <Windowing/Win32Window.h>

// RZE

#include <EngineCore/Input/InputHandler.h>
#include <Events/EventHandler.h>

#include <Windowing/WindowMessageAdaptor.h>
#include <Windowing/WinKeyCodes.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Conversions.h>

#include <stdlib.h>

// #TODO(Super test, just to get things working. Will need to handle this with an event OnTextInput or something)
#include <imGUI/imgui.h>

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

void Win32Window::SetTitle(const std::string& title)
{
	::SetWindowTextA(mOSWindowHandleData.windowHandle, (LPCSTR)title.c_str());
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
			WS_SYSMENU | WS_CAPTION, // @note WS_POPUP = borderless. WS_OVERLAPPEDWINDOW default
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
			LOG_CONSOLE("<Failure retrieving device context>");
			AssertFalse();
		}

		int pixelFormat = ChoosePixelFormat(mOSWindowHandleData.deviceContext, &mOSWindowHandleData.pixelFormatDesc);
		if (!pixelFormat)
		{
			LOG_CONSOLE("<Failure choosing pixel format>");
			AssertFalse();
		}

		if (!SetPixelFormat(mOSWindowHandleData.deviceContext, pixelFormat, &mOSWindowHandleData.pixelFormatDesc))
		{
			LOG_CONSOLE("<Failure setting pixel format>");
			AssertFalse();
		}

		InternalSetWindowPosition(Vector2D(0, 0));
		// #NOTE(Josh) Gonna put this here for now instead of in Engine.cpp until it has a better home
		SetWindowPos(GetConsoleWindow(), 0, 1024, 600, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
	}
}

void Win32Window::CompileInputMessages(InputHandler& inputHandler)
{
	// #TODO(Josh) Fix this later. Need to reset mouse wheel.
	if (inputHandler.GetMouseState().CurWheelVal != 0)
	{
		inputHandler.OnMouseWheel(0);
	}

	MSG msg;
	if (PeekMessage(&msg, mOSWindowHandleData.windowHandle, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
		case WM_CHAR:
		{
			if (ImGui::GetIO().WantCaptureKeyboard)
			{
				ImGui::GetIO().AddInputCharacter(static_cast<ImWchar>(msg.wParam));
			}
		}
		break;

		case WM_KEYDOWN:
		{
			const Int32 win32KeyCode = static_cast<Int32>(msg.wParam);
			const bool bIsRepeat = (msg.lParam & 0x40000000) != 0;

			Int32 key = ::MapVirtualKeyA(win32KeyCode, 2);
			if (key != 0)
			{
				inputHandler.OnKeyDown(key, bIsRepeat);
				TranslateMessage(&msg);
			}
		}
		break;

		case WM_KEYUP:
		{
			const Int32 win32KeyCode = static_cast<Int32>(::MapVirtualKeyA(static_cast<UINT>(msg.wParam), 2));
			inputHandler.OnKeyUp(win32KeyCode);
		}
		break;

		case WM_MOUSEMOVE:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseMove(xPos, yPos);
		}
		break;

		case WM_LBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseDown(EMouseButton::MouseButton_Left, xPos, yPos);
		}
		break;

		case WM_LBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Left, xPos, yPos);
		}
		break;

		case WM_MBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseDown(EMouseButton::MouseButton_Middle, xPos, yPos);
		}
		break;

		case WM_MBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Middle, xPos, yPos);
		}
		break;

		case WM_RBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseDown(EMouseButton::MouseButton_Right, xPos, yPos);
		}
		break;

		case WM_RBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Right, xPos, yPos);
		}
		break;

		case WM_MOUSEWHEEL:
		{
			Int32 delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
			inputHandler.OnMouseWheel(delta);
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
		ShowWindow(mOSWindowHandleData.windowHandle, SW_SHOWNORMAL); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default
	}
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
		GetClientRect(mOSWindowHandleData.windowHandle, &windowRect);
		int width = windowRect.right - windowRect.top;
		int height = windowRect.bottom - windowRect.top;

		if (width > 0 && height > 0)
		{
			windowEvent.mSizeX = width;
			windowEvent.mSizeY = height;
			eventHandler.PostWindowEvent(windowEvent);
		}
		else
		{
			LOG_CONSOLE("Setting 0 dimension window size. Could be minimized. Is this expected?");
		}
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
	::SetWindowPos(mOSWindowHandleData.windowHandle, nullptr, static_cast<int>(mPos.X()), static_cast<int>(mPos.Y()), static_cast<int>(size.X()), static_cast<int>(size.Y()), SWP_NOZORDER);
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


void Win32Window::SetWindowSize(const Vector2D& newSize)
{
	InternalSetWindowSize(newSize);
}

FilePath Win32Window::ShowOpenFilePrompt()
{
	//make sure this is commented out in all code (usually stdafx.h)
		// #define WIN32_LEAN_AND_MEAN 
	OPENFILENAME ofn;       // common dialog box structure
	TCHAR szFile[512] = { 0 };       // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = mOSWindowHandleData.windowHandle;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = (LPWSTR)"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		char test[512];
		size_t converted = 0;
		wcstombs_s(&converted, test, ofn.lpstrFile, 512);

		std::string path(test);
		size_t index = path.find("Assets\\");
		path = path.substr(index, path.size());

		return FilePath(path);
	}

	return FilePath();
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
