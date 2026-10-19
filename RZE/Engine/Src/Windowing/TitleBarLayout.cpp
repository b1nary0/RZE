#include <StdAfx.h>

#include <Windowing/TitleBarLayout.h>

// std lib
#include <algorithm>

ETitleBarHit TitleBarLayout::HitTest(int x, int y, int clientWidth, bool isMaximized, int resizeBorder) const
{
	if (y >= Height)
	{
		return ETitleBarHit::Client;
	}

	if (!isMaximized && y < resizeBorder)
	{
		if (x < resizeBorder)
		{
			return ETitleBarHit::ResizeTopLeft;
		}
		if (x >= clientWidth - resizeBorder)
		{
			return ETitleBarHit::ResizeTopRight;
		}
		return ETitleBarHit::ResizeTop;
	}

	if (MaximizeButton.Contains(x, y))
	{
		return ETitleBarHit::MaximizeButton;
	}

	const bool isOnControl = std::any_of(Controls.begin(), Controls.end(), [x, y](const Display::ScreenRect& control) { return control.Contains(x, y); });
	return isOnControl ? ETitleBarHit::Client : ETitleBarHit::Caption;
}
