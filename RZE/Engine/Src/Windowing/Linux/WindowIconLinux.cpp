#include <StdAfx.h>
#include <Windowing/Linux/WindowIconLinux.h>

#include <Windowing/WindowIcon.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

#include <SDL3/SDL.h>

#include <algorithm>

namespace
{
	// The largest icon made from an image; the desktop scales it down as it needs
	constexpr int k_maxIconSize = 256;
	constexpr const char* k_defaultIcon = "Assets/Application/RZE.png";
}

namespace WindowIconLinux
{
	bool Set(SDL_Window* window, const Filepath& image)
	{
		const IconImage source = WindowIcon::ReadImage(image);
		if (source.IsEmpty())
		{
			RZE_LOG_WARNING_ARGS("Couldn't read the window icon [%s]; keeping the window's icon.", image.GetRelativePath().c_str());
			return false;
		}

		IconImage square = WindowIcon::MakeSquare(source, std::min(k_maxIconSize, std::max(source.Width, source.Height)));
		// RGBA bytes, as the texels are; SDL copies them
		SDL_Surface* const surface = SDL_CreateSurfaceFrom(square.Width, square.Height, SDL_PIXELFORMAT_RGBA32, square.Texels.data(), square.Width * 4);
		const bool isSet = surface != nullptr && SDL_SetWindowIcon(window, surface);
		SDL_DestroySurface(surface);
		if (!isSet)
		{
			RZE_LOG_WARNING_ARGS("Couldn't make the window icon from [%s]; keeping the window's icon.", image.GetRelativePath().c_str());
		}
		return isSet;
	}

	bool SetDefault(SDL_Window* window)
	{
		return Set(window, Filepath(k_defaultIcon));
	}
}
