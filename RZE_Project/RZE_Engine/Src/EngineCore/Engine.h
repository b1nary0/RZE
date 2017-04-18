#pragma once

#include "Application.h"
#include "Events/EventHandler.h"
#include "Input/InputHandler.h"
#include "RenderCore/Renderer.h"

class Win32Window;

class RZE_Engine
{
	static UInt8 sInstanceCount;

public:

	RZE_Engine();
	~RZE_Engine();
	
	void Run(Functor<std::unique_ptr<RZE_Game>> createApplicationCallback);

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

	std::weak_ptr<Win32Window> GetMainWindow() const;

private:

	void Init();
	void PostInit(Functor<std::unique_ptr<RZE_Game>> createApplicationCallback);
	
	void Update();
	void ShutDown();

	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterInputEvents();

	std::unique_ptr<RZE_Game> mApplication;

	RZE_Renderer mRenderer;
	EventHandler mEventHandler;
	InputHandler mInputHandler;

	bool bShouldExit;
};
