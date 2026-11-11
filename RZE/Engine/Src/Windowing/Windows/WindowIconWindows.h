#pragma once

#include <Windowing/WindowIcon.h>

#include <memory>

#include <Windows.h>

class Filepath;

// A window's large (taskbar, Alt+Tab) and small (system menu) icons, made from an image file. They're destroyed with
// it, so it has to outlive the window's use of them.
class WindowIconWindows
{
public:
	~WindowIconWindows();

	WindowIconWindows(const WindowIconWindows&) = delete;
	WindowIconWindows& operator=(const WindowIconWindows&) = delete;

	// The image's icons at the system's icon sizes for dpi; null if the image can't be read
	static std::unique_ptr<WindowIconWindows> FromImage(const Filepath& image, UINT dpi);

	HICON GetLarge() const { return m_large; }
	HICON GetSmall() const { return m_small; }

private:
	WindowIconWindows(HICON largeIcon, HICON smallIcon);

	HICON m_large = nullptr;
	HICON m_small = nullptr;
};
