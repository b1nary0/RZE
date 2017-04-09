#pragma once

#include "Application.h"
#include "Events/EventHandler.h"
#include "Windowing/WindowManager.h"

class RZE_EngineCore
{

	static UInt8 sInstanceCount;

public:

	RZE_EngineCore();
	~RZE_EngineCore();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);

	void Run(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback);

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

private:

	void Init();
	void PostInit(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback);
	void Update();

	void ShutDown();

	void CompileEvents();

	void RegisterEvents();

	std::unique_ptr<RZE_Application> mApplication;
	std::weak_ptr<Win32Window> mMainWindow;

	WindowManager mWindowManager;
	EventHandler mEventHandler;

	bool bShouldExit;
};
