#pragma once

#include "Events/EventHandler.h"
#include "Windowing/WindowManager.h"

class RZE_EngineCore
{

public:

	RZE_EngineCore();
	~RZE_EngineCore();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);

	void Update();

	void ShutDown();

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

private:

	void Init();
	
	WindowManager mWindowManager;
	EventHandler mEventHandler;
};
