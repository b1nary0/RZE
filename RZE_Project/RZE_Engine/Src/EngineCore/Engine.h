#pragma once

#include "Application.h"
#include "Events/EventHandler.h"
#include "Input/InputHandler.h"

class Win32Window;

class RZE_EngineCore
{
	static UInt8 sInstanceCount;

public:

	RZE_EngineCore();
	~RZE_EngineCore();
	
	void Run(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback);

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

	std::weak_ptr<Win32Window> GetMainWindow() const;

private:

	void Init();
	void PostInit(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback);
	
	void Update();
	void ShutDown();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);
	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterInputEvents();

	std::unique_ptr<RZE_Application> mApplication;
	std::shared_ptr<Win32Window> mMainWindow;

	EventHandler mEventHandler;
	InputHandler mInputHandler;

	bool bShouldExit;
};
