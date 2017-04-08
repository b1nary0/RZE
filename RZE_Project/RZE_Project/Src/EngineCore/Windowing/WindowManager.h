#pragma once

// STL
#include <vector>
#include <memory>

class Win32Window;
class EventHandler;

class WindowManager
{

public:

	WindowManager();
	~WindowManager();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);

	void SetEventHandler(std::weak_ptr<EventHandler> eventHandler);

	void ProcessWindowEvents();

private:

	std::weak_ptr<EventHandler> mEventHandler;

	std::vector<std::shared_ptr<Win32Window>> mWindowList;
};