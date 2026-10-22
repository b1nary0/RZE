#include <StdAfx.h>

#include <Windowing/Win32Window.h>

// RZE

#include <EngineCore/Input/InputHandler.h>
#include <Events/EventHandler.h>

#include <Windowing/Display.h>
#include <Windowing/WindowIcon.h>
#include <Windowing/WindowMessageAdaptor.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>

#include <stdlib.h>

namespace
{
	// Used to link WinProc messages with the window without having to static other class instances etc
	WindowMessageAdaptor sWindowMessageAdaptor;

	// The executable's icon, from Editor/Resources/Editor.rc and Game/Resources/Game.rc
	constexpr WORD k_appIconResourceID = 1;

	// The exe's icon at the given system size, or Windows' default when the exe has none
	HICON LoadAppIcon(int widthMetric, int heightMetric)
	{
		HANDLE icon = LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(k_appIconResourceID), IMAGE_ICON, GetSystemMetrics(widthMetric), GetSystemMetrics(heightMetric), LR_DEFAULTCOLOR);
		return icon != nullptr ? static_cast<HICON>(icon) : LoadIcon(nullptr, IDI_APPLICATION);
	}
}

Win32Window::Win32Window(const WindowCreationParams& creationProtocol)
{
	m_title = creationProtocol.windowTitle;
	m_dimensions = Vector2D(static_cast<float>(creationProtocol.width), static_cast<float>(creationProtocol.height));

	Create(creationProtocol);
}

Win32Window::~Win32Window()
{
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
		GetClientRect(m_osWindowHandleData.windowHandle, &rcClip);
		MapWindowPoints(m_osWindowHandleData.windowHandle, GetParent(m_osWindowHandleData.windowHandle), (LPPOINT)(&rcClip), 1);
		ClipCursor(&rcClip);
		SetCursorPos(rcClip.left + static_cast<int>(m_dimensions.X() / 2), rcClip.top + static_cast<int>(m_dimensions.Y() / 2));
	}
}

void Win32Window::SetTitle(const std::string& title)
{
	m_title = title;
	::SetWindowTextA(m_osWindowHandleData.windowHandle, (LPCSTR)title.c_str());
}

bool Win32Window::SetIcon(const Filepath& image)
{
	const HWND windowHandle = m_osWindowHandleData.windowHandle;
	std::unique_ptr<WindowIcon> icon = WindowIcon::FromImage(image, GetDpiForWindow(windowHandle));
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
	m_icon = std::move(icon);
	return true;
}

const std::string& Win32Window::GetTitle() const
{
	return m_title;
}

const Vector2D& Win32Window::GetDimensions() const
{
	return m_dimensions;
}

const Vector2D& Win32Window::GetClientSize() const
{
	return m_clientSize;
}

void Win32Window::Create(const WindowCreationParams& creationProtocol)
{
	// Interface with WinAPI and create a window -- store useful information.
	RZE_LOG_ARGS("Creating window [%s]", m_title.c_str());

	const std::wstring wStrTitle = Conversions::StringToWString(m_title);

	//BYTE CursorMaskAND[] = { 0xFF };
	//BYTE CursorMaskXOR[] = { 0x00 };

	// @NOTE what do the properties that are set as 0 do?
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW; // @NOTE see https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176(v=vs.85).aspx
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadAppIcon(SM_CXICON, SM_CYICON);
	wc.hCursor = /*CreateCursor(NULL, 0, 0, 1, 1, CursorMaskAND, CursorMaskXOR);*/ LoadCursor(NULL, IDC_ARROW);  // @TODO fill this with proper customizable cursor
	wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1); // @NOTE what is this?
	wc.lpszMenuName = 0;
	wc.lpszClassName = wStrTitle.c_str(); // @NOTE does this represent what you think it represents?
	// Its own size, rather than Windows shrinking the large one
	wc.hIconSm = LoadAppIcon(SM_CXSMICON, SM_CYSMICON);

	if (RegisterClassEx(&wc))
	{
		m_osWindowHandleData.windowHandle = CreateWindowEx(0,
			wStrTitle.c_str(),
			wStrTitle.c_str(),
			WS_OVERLAPPEDWINDOW /*^ WS_SIZEBOX*/, // @note WS_POPUP = borderless. WS_OVERLAPPEDWINDOW default
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<int>(m_dimensions.X()),
			static_cast<int>(m_dimensions.Y()),
			0,
			0,
			GetModuleHandle(0),
			0);

		// So WindowProc can reach this window's state
		SetWindowLongPtr(m_osWindowHandleData.windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		m_osWindowHandleData.pixelFormatDesc =
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

		m_osWindowHandleData.deviceContext = GetDC(m_osWindowHandleData.windowHandle);
		if (!m_osWindowHandleData.deviceContext)
		{
			RZE_LOG_ERROR("<Failure retrieving device context>");
			AssertFalse();
		}

		int pixelFormat = ChoosePixelFormat(m_osWindowHandleData.deviceContext, &m_osWindowHandleData.pixelFormatDesc);
		if (!pixelFormat)
		{
			RZE_LOG_ERROR("<Failure choosing pixel format>");
			AssertFalse();
		}

		if (!SetPixelFormat(m_osWindowHandleData.deviceContext, pixelFormat, &m_osWindowHandleData.pixelFormatDesc))
		{
			RZE_LOG_ERROR("<Failure setting pixel format>");
			AssertFalse();
		}

		Display::MonitorInfo monitorInfo;
		if (!Display::GetMonitor(creationProtocol.monitorIndex, monitorInfo))
		{
			RZE_LOG_WARNING_ARGS("Launch monitor %d out of range (%d monitors). Using primary.", creationProtocol.monitorIndex, static_cast<int>(Display::EnumerateMonitors().size()));
			Display::GetMonitor(0, monitorInfo);
		}

		const Display::ScreenRect& workArea = monitorInfo.WorkArea;
		InternalSetWindowPosition(Vector2D(static_cast<float>(workArea.Left), static_cast<float>(workArea.Top)));
		// #NOTE(Josh) Gonna put this here for now instead of in Engine.cpp until it has a better home
		// A windowed app (the Editor) only has a console with -console
		if (HWND consoleWindow = GetConsoleWindow())
		{
			SetWindowPos(consoleWindow, 0, workArea.Left, workArea.Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
		}

		QueryClientSize();
	}
}

namespace
{
	// While a mouse button is down the window keeps the mouse, so the release still reaches it when it happens outside
	// the window (e.g. a drag that ends past its edge)
	void CaptureMouse(HWND windowHandle)
	{
		if (GetCapture() != windowHandle)
		{
			SetCapture(windowHandle);
		}
	}

	// buttonsDown is the button-up message's wParam, which says which buttons are still down
	void ReleaseMouseOnceUp(HWND windowHandle, WPARAM buttonsDown)
	{
		if ((buttonsDown & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0 && GetCapture() == windowHandle)
		{
			ReleaseCapture();
		}
	}
}

void Win32Window::CompileInputMessages(InputHandler& inputHandler)
{
	// The wheel only reports deltas, so clear it each frame. Check the proxy state: the raised state is
	// only refreshed by RaiseEvents(), so it stays stale while the application is stealing input.
	if (inputHandler.GetProxyMouseState().CurWheelVal != 0)
	{
		inputHandler.ClearMouseWheel();
	}

	// Every queued message, not one per frame: with frames capped, one per frame lets input queue up and lag
	MSG msg;
	while (PeekMessage(&msg, m_osWindowHandleData.windowHandle, 0, 0, PM_REMOVE))
	{
		if (m_hasCustomTitleBar && CompileNonClientMouseMessage(msg, inputHandler))
		{
			continue;
		}

		switch (msg.message)
		{
		case WM_CHAR:
		{
  			if (ImGui::GetIO().WantCaptureKeyboard)
  			{
  				ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(msg.wParam));
  			}
		}
		break;

		case WM_KEYDOWN:
		{
			const Int32 win32KeyCode = static_cast<Int32>(msg.wParam);
			const bool bIsRepeat = (msg.lParam & 0x40000000) != 0;

			inputHandler.OnKeyDown(win32KeyCode, bIsRepeat);
			Int32 key = ::MapVirtualKeyA(win32KeyCode, MAPVK_VK_TO_CHAR);
			if (key != 0)
			{
				TranslateMessage(&msg);
			}
		}
		break;

		case WM_KEYUP:
		{
			const Int32 win32KeyCode = static_cast<Int32>(msg.wParam);
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
			CaptureMouse(m_osWindowHandleData.windowHandle);
		}
		break;

		case WM_LBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Left, xPos, yPos);
			ReleaseMouseOnceUp(m_osWindowHandleData.windowHandle, msg.wParam);
		}
		break;

		case WM_MBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseDown(EMouseButton::MouseButton_Middle, xPos, yPos);
			CaptureMouse(m_osWindowHandleData.windowHandle);
		}
		break;

		case WM_MBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Middle, xPos, yPos);
			ReleaseMouseOnceUp(m_osWindowHandleData.windowHandle, msg.wParam);
		}
		break;

		case WM_RBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseDown(EMouseButton::MouseButton_Right, xPos, yPos);
			CaptureMouse(m_osWindowHandleData.windowHandle);
		}
		break;

		case WM_RBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			inputHandler.OnMouseUp(EMouseButton::MouseButton_Right, xPos, yPos);
			ReleaseMouseOnceUp(m_osWindowHandleData.windowHandle, msg.wParam);
		}
		break;

		case WM_XBUTTONDOWN:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			const EMouseButton::T button = GET_XBUTTON_WPARAM(msg.wParam) == XBUTTON1 ? EMouseButton::MouseButton_Back : EMouseButton::MouseButton_Forward;
			inputHandler.OnMouseDown(button, xPos, yPos);
			CaptureMouse(m_osWindowHandleData.windowHandle);
		}
		break;

		case WM_XBUTTONUP:
		{
			const Int32 xPos = GET_X_LPARAM(msg.lParam);
			const Int32 yPos = GET_Y_LPARAM(msg.lParam);

			const EMouseButton::T button = GET_XBUTTON_WPARAM(msg.wParam) == XBUTTON1 ? EMouseButton::MouseButton_Back : EMouseButton::MouseButton_Forward;
			inputHandler.OnMouseUp(button, xPos, yPos);
			// The low word holds the buttons still down, as for the other buttons
			ReleaseMouseOnceUp(m_osWindowHandleData.windowHandle, GET_KEYSTATE_WPARAM(msg.wParam));
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
	if (m_osWindowHandleData.windowHandle)
	{
		ShowWindow(m_osWindowHandleData.windowHandle, SW_SHOWNORMAL); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default
	}
}

bool Win32Window::IsFocused() const
{
	return GetForegroundWindow() == m_osWindowHandleData.windowHandle;
}

bool Win32Window::IsMinimised() const
{
	return IsIconic(m_osWindowHandleData.windowHandle) != FALSE;
}

bool Win32Window::IsMouseButtonDown() const
{
	// The high bit is set while the button is down
	return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0
		|| (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0
		|| (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0
		|| (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0
		|| (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
}

const Win32Window::OSWindowHandleData& Win32Window::GetOSWindowHandleData() const
{
	return m_osWindowHandleData;
}

void Win32Window::ProcessWinProcMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo, EventHandler& eventHandler)
{
	if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Destroy)
	{
		WindowEvent windowEvent(EWindowEventType::Window_Destroy);
		eventHandler.PostWindowEvent(windowEvent);
	}
	else if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Close)
	{
		WindowEvent windowEvent(EWindowEventType::Window_CloseRequested);
		eventHandler.PostWindowEvent(windowEvent);
	}
	else if (messageInfo.mMessageType == WindowMessageAdaptor::EMessageType::Window_Resize)
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

void Win32Window::InternalSetWindowPosition(const Vector2D& pos)
{
	m_pos = pos;
	::SetWindowPos(m_osWindowHandleData.windowHandle, 0, static_cast<int>(m_pos.X()), static_cast<int>(m_pos.Y()), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Win32Window::InternalSetWindowSize(const Vector2D& size)
{
	m_dimensions = size;
	::SetWindowPos(m_osWindowHandleData.windowHandle, nullptr, static_cast<int>(m_pos.X()), static_cast<int>(m_pos.Y()), static_cast<int>(size.X()), static_cast<int>(size.Y()), SWP_NOZORDER);
}

void Win32Window::QueryClientSize()
{
	RECT windowRect;
	GetClientRect(m_osWindowHandleData.windowHandle, &windowRect);
	int width = windowRect.right - windowRect.top;
	int height = windowRect.bottom - windowRect.top;

	m_clientSize.SetXY(static_cast<float>(width), static_cast<float>(height));
}

void Win32Window::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> windowResizeCallback([this](const Event& evt)
	{
		m_dimensions.SetX(evt.mWindowEvent.mSizeX);
		m_dimensions.SetY(evt.mWindowEvent.mSizeY);
	});

	eventHandler.RegisterForEvent(EEventType::Window, windowResizeCallback);
}


void Win32Window::SetWindowSize(const Vector2D& newSize)
{
	InternalSetWindowSize(newSize);
}

void Win32Window::Maximize()
{
	ShowWindow(m_osWindowHandleData.windowHandle, SW_MAXIMIZE);

	QueryClientSize();
}

void Win32Window::Minimize()
{
	ShowWindow(m_osWindowHandleData.windowHandle, SW_MINIMIZE);
}

void Win32Window::Restore()
{
	ShowWindow(m_osWindowHandleData.windowHandle, SW_RESTORE);

	QueryClientSize();
}

bool Win32Window::IsMaximised() const
{
	return IsZoomed(m_osWindowHandleData.windowHandle) != FALSE;
}

void Win32Window::RequestClose()
{
	PostMessage(m_osWindowHandleData.windowHandle, WM_CLOSE, 0, 0);
}

void Win32Window::SetCustomTitleBar(bool isCustom)
{
	m_hasCustomTitleBar = isCustom;

	// Has the OS work out the client area again (WM_NCCALCSIZE)
	SetWindowPos(m_osWindowHandleData.windowHandle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	QueryClientSize();
}

void Win32Window::SetTitleBarLayout(TitleBarLayout&& layout)
{
	m_titleBarLayout = std::move(layout);
}

int Win32Window::GetSystemTitleBarHeight() const
{
	const UINT dpi = GetDpiForWindow(m_osWindowHandleData.windowHandle);
	return GetSystemMetricsForDpi(SM_CYCAPTION, dpi) + GetFrameThickness();
}

float Win32Window::GetDpiScale() const
{
	return static_cast<float>(GetDpiForWindow(m_osWindowHandleData.windowHandle)) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
}

int Win32Window::GetFrameThickness() const
{
	const UINT dpi = GetDpiForWindow(m_osWindowHandleData.windowHandle);
	return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

LRESULT Win32Window::CalculateClientArea(WPARAM wParam, LPARAM lParam)
{
	const HWND windowHandle = m_osWindowHandleData.windowHandle;
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
		params.rgrc[0].top += GetFrameThickness();
	}
	return result;
}

LRESULT Win32Window::HitTest(WPARAM wParam, LPARAM lParam)
{
	const HWND windowHandle = m_osWindowHandleData.windowHandle;

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

	switch (m_titleBarLayout.HitTest(point.x, point.y, clientRect.right, IsZoomed(windowHandle) != FALSE, GetFrameThickness()))
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

bool Win32Window::CompileNonClientMouseMessage(const MSG& msg, InputHandler& inputHandler)
{
	const HWND windowHandle = m_osWindowHandleData.windowHandle;
	const auto toClient = [windowHandle](LPARAM screenPosition)
	{
		POINT point = { GET_X_LPARAM(screenPosition), GET_Y_LPARAM(screenPosition) };
		ScreenToClient(windowHandle, &point);
		return point;
	};

	switch (msg.message)
	{
	case WM_NCMOUSEMOVE:
	{
		const POINT point = toClient(msg.lParam);
		inputHandler.OnMouseMove(point.x, point.y);

		// For WM_NCMOUSELEAVE
		TRACKMOUSEEVENT track = { sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, windowHandle, 0 };
		TrackMouseEvent(&track);

		// The OS still sees it, e.g. to show Snap Layouts over the maximize button
		return false;
	}

	case WM_NCMOUSELEAVE:
	{
		// Wherever the mouse went: outside the client area if it left the window, so the UI stops showing it over the bar
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(windowHandle, &point);
		inputHandler.OnMouseMove(point.x, point.y);
		return true;
	}

	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	{
		// The maximize button is the application's: the OS would draw its own over it and track the click itself
		if (msg.wParam != HTMAXBUTTON)
		{
			return false;
		}
		const POINT point = toClient(msg.lParam);
		inputHandler.OnMouseDown(EMouseButton::MouseButton_Left, point.x, point.y);
		return true;
	}

	case WM_NCLBUTTONUP:
	{
		// Also when it's let go elsewhere on the bar after being pressed on the maximize button, so it isn't left down
		if (inputHandler.GetProxyMouseState().CurMouseBtnStates[EMouseButton::MouseButton_Left])
		{
			const POINT point = toClient(msg.lParam);
			inputHandler.OnMouseUp(EMouseButton::MouseButton_Left, point.x, point.y);
		}
		return msg.wParam == HTMAXBUTTON;
	}

	default:
		return false;
	}
}

LRESULT CALLBACK Win32Window::WindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	// @todo actually solve this instead of this hack
	// this is so that we can still use WM_SIZE for restore/maximize functionality but also
	// not get those called for every pixel the size changes.
	static bool k_isUserResizing = false;

	// Null until CreateWindowEx returns
	Win32Window* const owner = reinterpret_cast<Win32Window*>(GetWindowLongPtr(window, GWLP_USERDATA));

	// @unfinished - check for values of msg possibly needed
	switch (msg)
	{
	case WM_NCCALCSIZE:
	{
		if (owner != nullptr && owner->m_hasCustomTitleBar)
		{
			return owner->CalculateClientArea(wp, lp);
		}
		return DefWindowProc(window, msg, wp, lp);
	}

	case WM_NCHITTEST:
	{
		if (owner != nullptr && owner->m_hasCustomTitleBar)
		{
			return owner->HitTest(wp, lp);
		}
		return DefWindowProc(window, msg, wp, lp);
	}

	case WM_CREATE:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Create, wp, lp);
		return 0;
	}

	case WM_CLOSE:
	{
		// Not passed on to DefWindowProc, which would destroy the window: RZE_Application::OnCloseRequested decides
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Close, wp, lp);
		return 0;
	}

	case WM_DESTROY:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Destroy, wp, lp);
		return 0;
	}

	case WM_ENTERSIZEMOVE:
	{
		k_isUserResizing = true;

		return 0;
	}
	// #NOTE(Josh) This is fine for now, will make WM_SIZE it's own thing when necessary
	case WM_EXITSIZEMOVE:
	{
		sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Resize, wp, lp);

		k_isUserResizing = false;
		return 0;
	}
	
	case WM_SIZE:
	{
		if (!k_isUserResizing)
		{
			sWindowMessageAdaptor.PushMessage(WindowMessageAdaptor::EMessageType::Window_Resize, wp, lp);
		}

		return 0;
	}

	default:
	{
		return DefWindowProc(window, msg, wp, lp);
	}
	}
}
