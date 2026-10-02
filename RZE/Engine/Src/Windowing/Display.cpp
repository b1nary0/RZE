#include <StdAfx.h>

#include <Windowing/Display.h>

// winapi
#include <Windows.h>

// std lib
#include <algorithm>

namespace
{
	Display::ScreenRect ToScreenRect(const RECT& rect)
	{
		Display::ScreenRect screenRect;
		screenRect.Left = rect.left;
		screenRect.Top = rect.top;
		screenRect.Right = rect.right;
		screenRect.Bottom = rect.bottom;
		return screenRect;
	}

	BOOL CALLBACK EnumMonitorCallback(HMONITOR monitor, HDC, LPRECT, LPARAM userData)
	{
		std::vector<Display::MonitorInfo>& monitors = *reinterpret_cast<std::vector<Display::MonitorInfo>*>(userData);

		MONITORINFOEXA osInfo;
		osInfo.cbSize = sizeof(MONITORINFOEXA);
		if (GetMonitorInfoA(monitor, &osInfo))
		{
			Display::MonitorInfo info;
			info.DeviceName = osInfo.szDevice;
			info.Bounds = ToScreenRect(osInfo.rcMonitor);
			info.WorkArea = ToScreenRect(osInfo.rcWork);
			info.bIsPrimary = (osInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

			monitors.push_back(info);
		}

		// Continue enumeration
		return TRUE;
	}
}

namespace Display
{
	std::vector<MonitorInfo> EnumerateMonitors()
	{
		std::vector<MonitorInfo> monitors;
		EnumDisplayMonitors(nullptr, nullptr, EnumMonitorCallback, reinterpret_cast<LPARAM>(&monitors));

		// Guarantee primary monitor is index 0 while keeping OS order for the rest
		std::stable_partition(monitors.begin(), monitors.end(), [](const MonitorInfo& info)
		{
			return info.bIsPrimary;
		});

		return monitors;
	}

	bool GetMonitor(int index, MonitorInfo& outInfo)
	{
		const std::vector<MonitorInfo> monitors = EnumerateMonitors();
		if (index < 0 || index >= static_cast<int>(monitors.size()))
		{
			return false;
		}

		outInfo = monitors[index];
		return true;
	}
}
