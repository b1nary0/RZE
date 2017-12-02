#pragma once

#include <EngineApp.h>

#include <RZE_Config.h>

#include <Utils/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/SubSystemHandler.h>
#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Diotima/RenderSystem.h>
#include <Diotima/SceneCamera.h>

class GameWorld;
class Win32Window;

namespace Apollo
{
	class EntityComponentSystem;
}

class RZE_Engine
{
	static RZE_Engine* sInstance;

public:

	RZE_Engine();
	~RZE_Engine();
	
	static RZE_Engine* const Get()
	{
		if (!sInstance)
		{
			sInstance = new RZE_Engine();
		}

		return sInstance;
	}

	inline bool IsInitialized() { return bIsInitialized; }

	void Run(Functor<RZE_Game* const>& createApplicationCallback);

	WindowSettings&		GetWindowSettings();
	ResourceHandler&	GetResourceHandler();

private:

	void Init();
	void InitWorld();
	void PostInit(Functor<RZE_Game* const>& createApplicationCallback);

	void Update();

	void BeginShutDown();
	void InternalShutDown();
	void PostExit();

	void CompileEvents();

	void RegisterSubSystems();
	void RegisterWindowEvents();
	void RegisterInputEvents();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();

	void InitGame(Functor<RZE_Game* const> createGameCallback);

private:
	SubSystemHandler mSubSystemHandler;

private:
	RZE_Game* mApplication;
	GameWorld* mWorld;

	Win32Window* mMainWindow;

	ResourceHandler mResourceHandler;
	EventHandler mEventHandler;
	InputHandler mInputHandler;

	EngineConfig* mEngineConfig;

	bool bIsInitialized;
	bool bShouldExit;
};
