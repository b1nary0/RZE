#include <StdAfx.h>

#include <Windowing/Display.h>

#include <Windowing/SDLVideo.h>

#include <SDL3/SDL.h>

// std lib
#include <algorithm>

namespace
{
	Display::ScreenRect ToScreenRect(const SDL_Rect& rect)
	{
		Display::ScreenRect screenRect;
		screenRect.Left = rect.x;
		screenRect.Top = rect.y;
		screenRect.Right = rect.x + rect.w;
		screenRect.Bottom = rect.y + rect.h;
		return screenRect;
	}
}

namespace Display
{
	std::vector<MonitorInfo> EnumerateMonitors()
	{
		std::vector<MonitorInfo> monitors;
		const SDLVideo::Scope video;
		if (!video.IsAcquired())
		{
			return monitors;
		}

		const SDL_DisplayID primary = SDL_GetPrimaryDisplay();
		int count = 0;
		SDL_DisplayID* const displays = SDL_GetDisplays(&count);
		for (int index = 0; index < count; ++index)
		{
			SDL_Rect bounds;
			SDL_Rect workArea;
			if (!SDL_GetDisplayBounds(displays[index], &bounds) || !SDL_GetDisplayUsableBounds(displays[index], &workArea))
			{
				continue;
			}

			MonitorInfo info;
			const char* const name = SDL_GetDisplayName(displays[index]);
			info.DeviceName = name != nullptr ? name : "";
			info.Bounds = ToScreenRect(bounds);
			info.WorkArea = ToScreenRect(workArea);
			info.bIsPrimary = displays[index] == primary;
			monitors.push_back(info);
		}
		SDL_free(displays);

		// SDL already lists the primary first and the rest in the OS's order (EnumDisplayMonitors on Windows); this
		// keeps that true whatever SDL does
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

	double GetRefreshRate(void* windowHandle)
	{
		SDL_Window* const window = static_cast<SDL_Window*>(windowHandle);
		const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
		const SDL_DisplayMode* const mode = display != 0 ? SDL_GetCurrentDisplayMode(display) : nullptr;

		// 0 means SDL doesn't know it
		return mode != nullptr && mode->refresh_rate > 0.0f ? static_cast<double>(mode->refresh_rate) : 0.0;
	}
}
