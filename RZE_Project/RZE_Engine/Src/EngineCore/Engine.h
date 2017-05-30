#pragma once

#include "Application.h"
#include "Events/EventHandler.h"
#include "Input/InputHandler.h"

class Win32Window;
class GameWorld;

class RZE_Engine
{
public:

	RZE_Engine();
	~RZE_Engine();

	void Run(Functor<RZE_Game* const>& createApplicationCallback);

	void RegisterForEvent(const U16 eventType, Functor<void, const Event&> callback);
	void RegisterForInputEvent(const U16 eventType, Functor<void, U8> callback);

	GameWorld* const GetWorld() const;

	static RZE_Engine* const Get()
	{
		if (!sInstance)
		{
			sInstance = new RZE_Engine();
		}

		return sInstance;
	}

private:

	void Init();
	void InitWorld();
	void PostInit(Functor<RZE_Game* const>& createApplicationCallback);
	
	void Update();

	void BeginShutDown();
	void PostExit();

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

	static RZE_Engine* sInstance;

	bool bShouldExit;
};
