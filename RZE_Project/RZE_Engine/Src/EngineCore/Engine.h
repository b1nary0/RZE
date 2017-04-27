#pragma once

#include "Application.h"
#include "Events/EventHandler.h"
#include "Input/InputHandler.h"

class Win32Window;
class GameWorld;

class RZE_Engine
{
	static UInt8 sInstanceCount;

public:

	RZE_Engine();
	~RZE_Engine();
	
	void Run(Functor<RZE_Game* const> createApplicationCallback);

	void RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback);

	GameWorld* const GetWorld() const;

	// @todo does this have purpose here?
	//Win32Window* const GetMainWindow() const;

private:

	void Init();
	void InitWorld();
	void PostInit(Functor<RZE_Game* const> createApplicationCallback);
	
	void Update();
	void BeginShutDown();

	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterInputEvents();

	void CreateAndInitializeWindow();

	void InitGame(Functor<RZE_Game* const> createGameCallback);

	GameWorld* mWorld;

	Win32Window* mMainWindow;
	RZE_Game* mApplication;
	EventHandler mEventHandler;
	InputHandler mInputHandler;

	bool bShouldExit;
};
