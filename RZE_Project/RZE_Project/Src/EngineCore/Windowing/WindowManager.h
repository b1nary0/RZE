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

	void ProcessWindowEvents(EventHandler& eventHandler);

private:
	
	std::vector<std::shared_ptr<Win32Window>> mWindowList;
};