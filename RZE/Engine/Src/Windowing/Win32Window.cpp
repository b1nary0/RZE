#include <StdAfx.h>

#include <Windowing/Win32Window.h>

// RZE

#include <EngineCore/Input/InputHandler.h>
#include <Events/EventHandler.h>

#include <Windowing/WindowMessageAdaptor.h>

#include <Utils/Conversions.h>
#include <Utils/GUID.h>
#include <Utils/DebugUtils/Debug.h>

#include <stdlib.h>
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents

LRESULT CALLBACK WinProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp);

namespace
{
	// Used to link WinProc messages with the window without having to static other class instances etc
	WindowMessageAdaptor sWindowMessageAdaptor;
}

Win32Window::Win32Window(const WindowCreationParams& creationProtocol)
{
	m_title = creationProtocol.windowTitle;
	m_dimensions = Vector2D(static_cast<float>(creationProtocol.width), static_cast<float>(creationProtocol.height));

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
		GetClientRect(m_osWindowHandleData.windowHandle, &rcClip);
		MapWindowPoints(m_osWindowHandleData.windowHandle, GetParent(m_osWindowHandleData.windowHandle), (LPPOINT)(&rcClip), 1);
		ClipCursor(&rcClip);
		SetCursorPos(rcClip.left + static_cast<int>(m_dimensions.X() / 2), rcClip.top + static_cast<int>(m_dimensions.Y() / 2));
	}
}

void Win32Window::SetTitle(const std::string& title)
{
	::SetWindowTextA(m_osWindowHandleData.windowHandle, (LPCSTR)title.c_str());
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
	LOG_CONSOLE_ANNOUNCE("[ Creating Win32Window ] Title : %s", m_title.c_str());

	const std::wstring wStrTitle = Conversions::StringToWString(m_title);

	//BYTE CursorMaskAND[] = { 0xFF };
	//BYTE CursorMaskXOR[] = { 0x00 };

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
			RZE_LOG("<Failure retrieving device context>");
			AssertFalse();
		}

		int pixelFormat = ChoosePixelFormat(m_osWindowHandleData.deviceContext, &m_osWindowHandleData.pixelFormatDesc);
		if (!pixelFormat)
		{
			RZE_LOG("<Failure choosing pixel format>");
			AssertFalse();
		}

		if (!SetPixelFormat(m_osWindowHandleData.deviceContext, pixelFormat, &m_osWindowHandleData.pixelFormatDesc))
		{
			RZE_LOG("<Failure setting pixel format>");
			AssertFalse();
		}

		InternalSetWindowPosition(Vector2D(0, 0));
		// #NOTE(Josh) Gonna put this here for now instead of in Engine.cpp until it has a better home
		SetWindowPos(GetConsoleWindow(), 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

		QueryClientSize();
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
	if (PeekMessage(&msg, m_osWindowHandleData.windowHandle, 0, 0, PM_REMOVE))
	{
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
	if (m_osWindowHandleData.windowHandle)
	{
		ShowWindow(m_osWindowHandleData.windowHandle, SW_SHOWNORMAL); // @note SW_SHOWMAXIMIZED for borderless fullscreen. SW_SHOWDEFAULT default
	}
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
			RZE_LOG("Setting 0 dimension window size. Could be minimized. Is this expected?");
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

// @TODO pass in some like enum EFileOpenContext or better yet a struct of instructions for the open file prompt
bool Win32Window::ShowOpenFilePrompt(const FilePromptParams& params, std::string& chosenPath)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			static GUID guid = GUIDHelper::GenerateGUID();
			pFileOpen->SetClientGuid(guid);
			pFileOpen->SetFileName(Conversions::StringToWString(params.DirectoryPath.GetAbsolutePath()).c_str());
			{
				IShellItem* folder;
				const HRESULT result = SHCreateItemFromParsingName(
					Conversions::StringToWString(params.DirectoryPath.GetAbsolutePath()).c_str(), 
					nullptr, 
					IID_PPV_ARGS(&folder));

				if (SUCCEEDED(result))
				{
					pFileOpen->SetFolder(folder);
				}
			}

			std::wstring wStrPromptName = Conversions::StringToWString(params.Name);
			std::wstring wStrTypeFilter = Conversions::StringToWString(params.FiletypeFilter);

			COMDLG_FILTERSPEC rgSpec[] =
			{
				{wStrPromptName.c_str(), wStrTypeFilter.c_str()}
			};
			pFileOpen->SetFileTypes(1, rgSpec);

			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					constexpr size_t k_maxCharacters = 512;

					char converted[k_maxCharacters];
					size_t convertedCount = 0;
					wcstombs_s(&convertedCount, converted, pszFilePath, k_maxCharacters);

					chosenPath = converted;

					pItem->Release();
					pFileOpen->Release();
					CoUninitialize();

					return true;
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}

	return false;
}

bool Win32Window::ShowSaveFilePrompt(const FilePromptParams& params, std::string& chosenPath)
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileSaveDialog* pSaveFile;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
			IID_IFileSaveDialog, reinterpret_cast<void**>(&pSaveFile));

		if (SUCCEEDED(hr))
		{
			static GUID guid = GUIDHelper::GenerateGUID();
			pSaveFile->SetClientGuid(guid);

			std::wstring wStrPromptName = Conversions::StringToWString(params.Name);
			std::wstring wStrTypeFilter = Conversions::StringToWString(params.FiletypeFilter);
			COMDLG_FILTERSPEC rgSpec[] =
			{
				{wStrPromptName.c_str(), wStrTypeFilter.c_str()}
			};
			
			pSaveFile->SetFileTypes(1, rgSpec);

			hr = pSaveFile->Show(m_osWindowHandleData.windowHandle);
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem = nullptr;
				pSaveFile->GetResult(&pItem);

				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				constexpr size_t k_maxCharacters = 512;

				char converted[k_maxCharacters];
				size_t convertedCount = 0;
				wcstombs_s(&convertedCount, converted, pszFilePath, k_maxCharacters);

				chosenPath = converted;
				
				pItem->Release();
				pSaveFile->Release();
				CoUninitialize();

				return true;
			}
			pSaveFile->Release();
		}
		CoUninitialize();
	}

	return false;
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
