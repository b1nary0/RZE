#include <Windowing/SDLVideo.h>

#include <SDL3/SDL.h>

namespace SDLVideo
{
	bool Acquire()
	{
		// Before SDL first starts, since some are only read then. Each keeps what RZE did before it used SDL, where
		// SDL's default is different.
		static const bool s_hintsSet = []()
			{
				// The process has never declared itself DPI aware, so Windows scales it on high-DPI monitors
				SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "unaware");
				// Alt and F10 still open the window menu (and Alt+Space its menu), as with DefWindowProc
				SDL_SetHint(SDL_HINT_WINDOWS_ENABLE_MENU_MNEMONICS, "1");
				// The screensaver and display sleep still come on
				SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
				// The click that focuses the window still reaches it, e.g. presses the button under the mouse
				SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
				// Ctrl+C and SIGTERM still end the process, rather than becoming a quit event nothing reads
				SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
				// Our own main() needs no SDL_main.h or SDL_SetMainReady: SDL only needs those where it must run main
				// itself (e.g. iOS), not on Windows or Linux
				return true;
			}();
		(void)s_hintsSet;

		if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			RZE_LOG_ERROR("Couldn't start SDL video: " << SDL_GetError());
			return false;
		}
		return true;
	}

	void Release()
	{
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}
