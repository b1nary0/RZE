#pragma once

// How the main loop paces its frames. Engine.ini's [FrameSettings] gives the defaults (EngineConfig), and
// RZE_Engine::SetFrameSettings changes them while running.
struct FrameSettings
{
	// Present on the display's refresh
	bool VSync = true;
	// The most frames per second the loop runs; 0 for no cap beyond vsync
	double MaxFrameRate = 0.0;
	// Frames per second while the window is unfocused or minimised; 0 to not throttle
	double UnfocusedFrameRate = 30.0;
};
