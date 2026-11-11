#pragma once

struct SDL_Window;
class Filepath;

// A window's icon, made from an image file. A Linux executable has no icon of its own (Windows takes RZE.ico from
// the executable's resources), so each window, the splash screen's among them, is given the default one when made.
namespace WindowIconLinux
{
	// False if image couldn't be read or made an icon, which leaves the icon as it was
	bool Set(SDL_Window* window, const Filepath& image);
	// RZE's own icon: RZE.ico's largest image
	bool SetDefault(SDL_Window* window);
}
