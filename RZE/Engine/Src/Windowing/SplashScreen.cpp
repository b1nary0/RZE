#include <StdAfx.h>

#include <Windowing/SplashScreen.h>

#include <Windowing/Display.h>
#include <Windowing/WindowIcon.h>

#include <Utils/Conversions.h>

#include <dwmapi.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace
{
	constexpr wchar_t k_windowClassName[] = L"RZE_SplashScreen";

	// Sent by the engine, once start-up is done. The window ignores WM_CLOSE, so Alt+F4 doesn't close it.
	constexpr UINT k_closeMessage = WM_APP;

	// At 96 DPI. The image's texels are pixels at 96 DPI too, so everything scales with the DPI together.
	constexpr int k_width = 400;
	constexpr int k_height = 225;
	constexpr int k_statusMargin = 14;
	constexpr int k_statusPointSize = 9;
	// Under the status, which leaves room for it whether it's shown or not
	constexpr int k_progressBarHeight = 3;
	constexpr int k_progressBarGap = 6;

	// The Editor's window background (EditorApp::StyleSetup's WindowBg)
	constexpr COLORREF k_backgroundColour = RGB(33, 33, 33);
	constexpr COLORREF k_statusColour = RGB(150, 150, 150);
	// The Editor's progress bars (EditorApp::StyleSetup's PlotHistogram) on a track a little lighter than the background
	constexpr COLORREF k_progressColour = RGB(230, 179, 0);
	constexpr COLORREF k_progressTrackColour = RGB(55, 55, 55);

	// SetStatus without a progress bar
	constexpr float k_noProgress = -1.0f;

	constexpr int k_bytesPerTexel = 4;

	// Windows 11's DWMWA_WINDOW_CORNER_PREFERENCE and DWMWCP_ROUND, which the project's Windows SDK (10.0.18362) predates.
	// Windows 10 ignores the attribute.
	constexpr DWORD k_windowCornerPreferenceAttribute = 33;
	constexpr DWORD k_roundCorners = 2;

	// Adds weight of image's texel at (x, y) to premultiplied: red, green and blue premultiplied by alpha, then alpha,
	// all out of 255
	void AddTexel(const IconImage& image, int x, int y, double weight, double(&premultiplied)[4])
	{
		const size_t index = (static_cast<size_t>(y) * image.Width + x) * k_bytesPerTexel;
		const double alpha = image.Texels[index + 3];
		for (int channel = 0; channel < 3; ++channel)
		{
			premultiplied[channel] += weight * image.Texels[index + channel] * alpha / 255.0;
		}
		premultiplied[3] += weight * alpha;
	}

	// image scaled to width x height (bilinear) and blended over canvas at (left, top). canvas is opaque BGRA texels,
	// top row first.
	void DrawImage(const IconImage& image, int left, int top, int width, int height, std::vector<U8>& canvas, int canvasWidth, int canvasHeight)
	{
		const double scaleX = static_cast<double>(image.Width) / width;
		const double scaleY = static_cast<double>(image.Height) / height;

		for (int y = std::max(0, -top); y < height && top + y < canvasHeight; ++y)
		{
			const double sourceY = std::clamp((y + 0.5) * scaleY - 0.5, 0.0, image.Height - 1.0);
			const int y0 = static_cast<int>(sourceY);
			const int y1 = std::min(y0 + 1, image.Height - 1);
			const double fractionY = sourceY - y0;

			for (int x = std::max(0, -left); x < width && left + x < canvasWidth; ++x)
			{
				const double sourceX = std::clamp((x + 0.5) * scaleX - 0.5, 0.0, image.Width - 1.0);
				const int x0 = static_cast<int>(sourceX);
				const int x1 = std::min(x0 + 1, image.Width - 1);
				const double fractionX = sourceX - x0;

				// Premultiplied, so transparent texels don't darken the edges
				double premultiplied[4] = {};
				AddTexel(image, x0, y0, (1.0 - fractionX) * (1.0 - fractionY), premultiplied);
				AddTexel(image, x1, y0, fractionX * (1.0 - fractionY), premultiplied);
				AddTexel(image, x0, y1, (1.0 - fractionX) * fractionY, premultiplied);
				AddTexel(image, x1, y1, fractionX * fractionY, premultiplied);

				U8* const destination = &canvas[(static_cast<size_t>(top + y) * canvasWidth + left + x) * k_bytesPerTexel];
				const double remaining = 1.0 - premultiplied[3] / 255.0;
				destination[0] = static_cast<U8>(std::lround(premultiplied[2] + destination[0] * remaining));
				destination[1] = static_cast<U8>(std::lround(premultiplied[1] + destination[1] * remaining));
				destination[2] = static_cast<U8>(std::lround(premultiplied[0] + destination[2] * remaining));
			}
		}
	}
}

struct SplashScreen::State
{
	// The thread's body: makes and shows the window, then handles its messages until it closes
	void Run();
	// Null if it couldn't be made
	HWND CreateSplashWindow();
	// The background with the image on it, width x height
	void BuildCanvas(int width, int height);
	void Paint(HWND window);

	static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	// Set before the thread starts
	IconImage Image;
	std::wstring Title;
	int MonitorIndex = 0;

	std::thread Thread;

	// Set once the window is on screen, or couldn't be made
	std::mutex ReadyMutex;
	std::condition_variable ReadyCondition;
	bool IsReady = false;
	HWND Window = nullptr;

	std::mutex StatusMutex;
	std::wstring Status;
	// k_noProgress for no progress bar
	float Progress = k_noProgress;

	// Only the splash screen's thread uses these
	UINT Dpi = USER_DEFAULT_SCREEN_DPI;
	HFONT Font = nullptr;
	std::vector<U8> Canvas;
	int CanvasWidth = 0;
	int CanvasHeight = 0;
};

void SplashScreen::State::Run()
{
	OPTICK_THREAD("Splash Screen");

	const HWND window = CreateSplashWindow();
	{
		std::lock_guard lock(ReadyMutex);
		Window = window;
		IsReady = true;
	}
	ReadyCondition.notify_all();

	if (window == nullptr)
	{
		return;
	}

	MSG message;
	while (GetMessage(&message, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	DeleteObject(Font);
	UnregisterClass(k_windowClassName, GetModuleHandle(nullptr));
}

HWND SplashScreen::State::CreateSplashWindow()
{
	const HINSTANCE instance = GetModuleHandle(nullptr);

	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_APPSTARTING);
	windowClass.lpszClassName = k_windowClassName;
	if (!RegisterClassEx(&windowClass))
	{
		return nullptr;
	}

	Display::MonitorInfo monitorInfo;
	if (!Display::GetMonitor(MonitorIndex, monitorInfo))
	{
		Display::GetMonitor(0, monitorInfo);
	}
	const Display::ScreenRect& workArea = monitorInfo.WorkArea;

	// Hidden and tiny at first, but on its monitor, so it can size itself for that monitor's DPI. A tool window, so it
	// has no taskbar button: the main window's, there from when it's made (cloaked), is the application's.
	const HWND window = CreateWindowEx(WS_EX_TOOLWINDOW, k_windowClassName, Title.c_str(), WS_POPUP,
		workArea.Left + workArea.Width() / 2, workArea.Top + workArea.Height() / 2, 1, 1,
		nullptr, nullptr, instance, nullptr);
	if (window == nullptr)
	{
		UnregisterClass(k_windowClassName, instance);
		return nullptr;
	}

	// So WindowProc can reach this
	SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	Dpi = GetDpiForWindow(window);
	const int width = MulDiv(k_width, Dpi, USER_DEFAULT_SCREEN_DPI);
	const int height = MulDiv(k_height, Dpi, USER_DEFAULT_SCREEN_DPI);
	BuildCanvas(width, height);
	Font = CreateFont(-MulDiv(k_statusPointSize, Dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

	// Windows 11 leaves popups square-cornered unless they ask
	DwmSetWindowAttribute(window, k_windowCornerPreferenceAttribute, &k_roundCorners, sizeof(k_roundCorners));

	SetWindowPos(window, nullptr,
		workArea.Left + (workArea.Width() - width) / 2, workArea.Top + (workArea.Height() - height) / 2, width, height,
		SWP_NOZORDER | SWP_NOACTIVATE);
	ShowWindow(window, SW_SHOWNORMAL);
	// Paints it now, so it's on screen before the constructor returns
	UpdateWindow(window);

	return window;
}

void SplashScreen::State::BuildCanvas(int width, int height)
{
	CanvasWidth = width;
	CanvasHeight = height;
	Canvas.resize(static_cast<size_t>(width) * height * k_bytesPerTexel);
	for (size_t texel = 0; texel < Canvas.size(); texel += k_bytesPerTexel)
	{
		Canvas[texel + 0] = GetBValue(k_backgroundColour);
		Canvas[texel + 1] = GetGValue(k_backgroundColour);
		Canvas[texel + 2] = GetRValue(k_backgroundColour);
		Canvas[texel + 3] = 255;
	}

	if (Image.IsEmpty())
	{
		return;
	}

	const int imageWidth = MulDiv(Image.Width, Dpi, USER_DEFAULT_SCREEN_DPI);
	const int imageHeight = MulDiv(Image.Height, Dpi, USER_DEFAULT_SCREEN_DPI);
	DrawImage(Image, (width - imageWidth) / 2, (height - imageHeight) / 2, imageWidth, imageHeight, Canvas, width, height);
}

void SplashScreen::State::Paint(HWND window)
{
	PAINTSTRUCT paint;
	const HDC windowDC = BeginPaint(window, &paint);

	// Drawn off screen first, so the status doesn't flicker as it changes
	const HDC bufferDC = CreateCompatibleDC(windowDC);
	const HBITMAP buffer = CreateCompatibleBitmap(windowDC, CanvasWidth, CanvasHeight);
	const HGDIOBJ previousBitmap = SelectObject(bufferDC, buffer);

	BITMAPINFO canvasInfo = {};
	canvasInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	canvasInfo.bmiHeader.biWidth = CanvasWidth;
	// Negative for top row first, as the canvas is
	canvasInfo.bmiHeader.biHeight = -CanvasHeight;
	canvasInfo.bmiHeader.biPlanes = 1;
	canvasInfo.bmiHeader.biBitCount = 32;
	canvasInfo.bmiHeader.biCompression = BI_RGB;
	SetDIBitsToDevice(bufferDC, 0, 0, CanvasWidth, CanvasHeight, 0, 0, 0, CanvasHeight, Canvas.data(), &canvasInfo, DIB_RGB_COLORS);

	std::wstring status;
	float progress = k_noProgress;
	{
		std::lock_guard lock(StatusMutex);
		status = Status;
		progress = Progress;
	}

	const HGDIOBJ previousFont = SelectObject(bufferDC, Font);
	SetBkMode(bufferDC, TRANSPARENT);
	SetTextColor(bufferDC, k_statusColour);
	const int margin = MulDiv(k_statusMargin, Dpi, USER_DEFAULT_SCREEN_DPI);
	const int barHeight = MulDiv(k_progressBarHeight, Dpi, USER_DEFAULT_SCREEN_DPI);
	const int barTop = CanvasHeight - margin - barHeight;
	RECT statusRect = { margin, 0, CanvasWidth - margin, barTop - MulDiv(k_progressBarGap, Dpi, USER_DEFAULT_SCREEN_DPI) };
	DrawText(bufferDC, status.c_str(), static_cast<int>(status.size()), &statusRect, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

	if (progress >= 0.0f)
	{
		RECT barRect = { margin, barTop, CanvasWidth - margin, barTop + barHeight };
		const HBRUSH trackBrush = CreateSolidBrush(k_progressTrackColour);
		FillRect(bufferDC, &barRect, trackBrush);
		DeleteObject(trackBrush);

		barRect.right = barRect.left + static_cast<int>(std::lround((barRect.right - barRect.left) * std::min(progress, 1.0f)));
		const HBRUSH progressBrush = CreateSolidBrush(k_progressColour);
		FillRect(bufferDC, &barRect, progressBrush);
		DeleteObject(progressBrush);
	}

	BitBlt(windowDC, 0, 0, CanvasWidth, CanvasHeight, bufferDC, 0, 0, SRCCOPY);

	SelectObject(bufferDC, previousFont);
	SelectObject(bufferDC, previousBitmap);
	DeleteObject(buffer);
	DeleteDC(bufferDC);

	EndPaint(window, &paint);
}

LRESULT CALLBACK SplashScreen::State::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* const state = reinterpret_cast<State*>(GetWindowLongPtr(window, GWLP_USERDATA));

	switch (message)
	{
	case WM_PAINT:
		if (state != nullptr)
		{
			state->Paint(window);
			return 0;
		}
		break;
	case WM_ERASEBKGND:
		// WM_PAINT covers all of it
		return 1;
	case WM_CLOSE:
		return 0;
	case k_closeMessage:
		DestroyWindow(window);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

SplashScreen::SplashScreen(const Filepath& image, const std::string& title, int monitorIndex)
	: m_state(std::make_unique<State>())
{
	m_state->Image = WindowIcon::ReadImage(image);
	if (m_state->Image.IsEmpty())
	{
		RZE_LOG_WARNING_ARGS("Couldn't read the splash screen image [%s].", image.GetRelativePath().c_str());
	}
	m_state->Title = Conversions::StringToWString(title);
	m_state->MonitorIndex = monitorIndex;

	State* const state = m_state.get();
	m_state->Thread = std::thread([state]()
		{
			state->Run();
		});

	std::unique_lock lock(m_state->ReadyMutex);
	m_state->ReadyCondition.wait(lock, [state]() { return state->IsReady; });
}

SplashScreen::~SplashScreen()
{
	if (m_state->Window != nullptr)
	{
		PostMessage(m_state->Window, k_closeMessage, 0, 0);
	}
	m_state->Thread.join();
}

void SplashScreen::SetStatus(const std::string& status)
{
	SetStatus(status, k_noProgress);
}

void SplashScreen::SetStatus(const std::string& status, float progress)
{
	const std::wstring wideStatus = Conversions::StringToWString(status);
	{
		std::lock_guard lock(m_state->StatusMutex);
		// Loading sets it every pass, mostly to what it already is
		if (wideStatus == m_state->Status && progress == m_state->Progress)
		{
			return;
		}
		m_state->Status = wideStatus;
		m_state->Progress = progress;
	}

	// Its thread repaints it
	if (m_state->Window != nullptr)
	{
		InvalidateRect(m_state->Window, nullptr, FALSE);
	}
}
