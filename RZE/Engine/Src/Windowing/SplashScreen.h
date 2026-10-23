#pragma once

// std lib
#include <memory>
#include <string>

class Filepath;

// A small window with an image and what start-up is doing, shown while the engine loads. It has its own thread and
// message loop, so it keeps drawing (and Windows doesn't call it not responding) while the main thread is busy.
class SplashScreen
{
public:
	// Shows image centred on monitorIndex's work area (the primary monitor's if there's no such monitor), and returns
	// once it's on screen. Without the image it shows just the status.
	SplashScreen(const Filepath& image, const std::string& title, int monitorIndex);
	// Closes it
	~SplashScreen();

	SplashScreen(const SplashScreen&) = delete;
	SplashScreen& operator=(const SplashScreen&) = delete;

	// What start-up is doing now, shown along the bottom. Safe from any thread.
	void SetStatus(const std::string& status);
	// With a progress bar under it, progress from 0 to 1
	void SetStatus(const std::string& status, float progress);

private:
	// Shared with the splash screen's thread
	struct State;
	std::unique_ptr<State> m_state;
};
