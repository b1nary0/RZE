#pragma once

// std lib
#include <string>
#include <vector>

// Thin layer over the OS display/monitor API so windowing code doesn't deal with platform handles directly.
namespace Display
{
	struct ScreenRect
	{
		int Left{ 0 };
		int Top{ 0 };
		int Right{ 0 };
		int Bottom{ 0 };

		int Width() const { return Right - Left; }
		int Height() const { return Bottom - Top; }
	};

	struct MonitorInfo
	{
		std::string DeviceName;		// e.g. \\.\DISPLAY2
		ScreenRect Bounds;			// Full monitor area in virtual-desktop coordinates
		ScreenRect WorkArea;		// Monitor area excluding taskbar etc
		bool bIsPrimary{ false };
	};

	// Primary monitor is always index 0, remaining monitors follow in OS enumeration order.
	std::vector<MonitorInfo> EnumerateMonitors();

	// Returns false if index is out of range.
	bool GetMonitor(int index, MonitorInfo& outInfo);
}
