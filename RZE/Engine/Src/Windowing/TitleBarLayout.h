#pragma once

// std lib
#include <vector>

// RZE
#include <Windowing/Display.h>

// What a point in the window is over, as far as a title bar the application draws itself is concerned
enum class ETitleBarHit
{
	Client,
	// Drags the window; double-clicking maximizes it
	Caption,
	MaximizeButton,
	ResizeTop,
	ResizeTopLeft,
	ResizeTopRight
};

// A title bar the application draws itself in place of the OS's (Win32Window::SetCustomTitleBar): where it is and which
// parts of it take clicks, so the window can tell the OS which parts move the window. In client pixels.
struct TitleBarLayout
{
	// From the top of the window. 0 means there's no bar, so all of it is client area.
	int Height{ 0 };
	// Parts of the bar that take clicks themselves (menus, buttons). The rest of the bar moves the window.
	std::vector<Display::ScreenRect> Controls;
	// Reported to the OS as its maximize button, so Windows 11 shows Snap Layouts while it's hovered
	Display::ScreenRect MaximizeButton;

	// resizeBorder is how far in from the top edge (and, at the top corners, the sides) resizes the window. There's no
	// resizing while it's maximized.
	ETitleBarHit HitTest(int x, int y, int clientWidth, bool isMaximized, int resizeBorder) const;
};
