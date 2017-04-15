#include "StdAfx.h"

#include "Windowing/WindowManager.h"
#include "Windowing/Win32Window.h"

#include "Events/EventHandler.h"

WindowManager::WindowManager()
{
}

WindowManager::~WindowManager()
{
}

std::weak_ptr<Win32Window> WindowManager::MakeWindow(const std::string& title, const int width, const int height)
{
	assert(!title.empty());

	const int windowID = mWindowList.size();

	Win32Window::WindowCreationProtocol creationProtocol;
	creationProtocol.WindowID = windowID;
	creationProtocol.WindowTitle = title;
	creationProtocol.Width = width;
	creationProtocol.Height = height;

	Win32Window* window = new Win32Window(creationProtocol);

	mWindowList.push_back(std::shared_ptr<Win32Window>(window));
	return mWindowList[windowID];
}

void WindowManager::CompileEvents(EventHandler& eventHandler)
{
	for (auto& window : mWindowList)
	{	
		window->CompileMessages(eventHandler);
	}
}
