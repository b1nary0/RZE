#pragma once

// winapi
// @improvement does it suffice to have a pch here or refactor to avoid this in a .h?
#include <Windows.h>

// std lib
#include <memory>
#include <string>

// RZE
#include "WindowMessageAdaptor.h"
#include "../Utils/Math/Vector2D.h"

class EventHandler;

class Win32Window
{
	friend class WindowManager;

private:

	struct WindowCreationProtocol
	{
		std::string		WindowTitle;
		int				WindowID{ -1 };
		int				Width{ -1 };
		int				Height{ -1 };
	};

	struct OSWindowHandleData
	{
		HWND mWindowHandle;
	};

public:

	const std::string& GetTitle() const;
	const Vector2D& GetDimensions() const;

protected:

private:

	void Create(const WindowCreationProtocol& creationProtocol);

	void ProcessWindowMessages(std::weak_ptr<EventHandler> eventHandler);
	void ProcessWindowMessage(const WindowMessageAdaptor::WindowMessageInfo& messageInfo);

	Win32Window(const WindowCreationProtocol& creationProtocol);
	
	OSWindowHandleData mOSWindowHandleData;
	WindowCreationProtocol mCreationData;

	int mWindowID;
	std::string mTitle;
	Vector2D mDimensions;
};
