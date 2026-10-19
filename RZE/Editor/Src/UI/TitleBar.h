#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

#include <Windowing/TitleBarLayout.h>

class Win32Window;

namespace Editor
{
	// What makes the main menu bar the window's title bar, in place of the OS's (Win32Window::SetCustomTitleBar): the
	// logo before the menus, then the window's title centred in what's left, and the minimize, maximize and close
	// buttons. Each UI frame it tells the window which parts of the bar move it and which take clicks.
	class TitleBar
	{
	public:
		TitleBar() = default;
		~TitleBar() = default;

	public:
		// While resources can be loaded and released
		void Initialize();
		void ShutDown();

		// First thing in the main menu bar
		void Begin(const Win32Window& window);
		// A part of the bar, from left to right, that takes clicks itself, e.g. the menus
		void AddControl(float left, float right);
		// Last thing in the main menu bar: the title and the window's buttons. Then gives the window the bar's layout.
		void End(Win32Window& window);

	private:
		void DrawLogo(const Win32Window& window);
		void DrawTitle(const Win32Window& window, float right);
		// Minimize, maximize (restore while maximized) and close, at the bar's right end
		void DrawWindowButtons(Win32Window& window);

	private:
		// RZE/logo.png
		ResourceHandle m_logo;
		// Built up through the frame
		TitleBarLayout m_layout;
	};
}
