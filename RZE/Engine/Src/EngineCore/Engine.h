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
	Diotima::SceneCamera& GetSceneCamera();

	Diotima::RenderSystem* GetRenderSystem()
	{
		return mSubSystemHandler.GetSubSystemByIndex<Diotima::RenderSystem>(mRenderSystemId);
	}

	Apollo::EntityComponentSystem* GetECS();

	const Vector2D& GetMainWindowSize() const;

	inline float GetDeltaTime() const { return mDeltaTime; }

	void SetCursorEnabled(bool enabled);

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

	inline void SetDisplayDebugServices(bool bShouldDisplay) { bDisplayDebugServices = bShouldDisplay; }
	inline bool ShouldDisplayDebugServices() { return bDisplayDebugServices; }

	float CalculateAvgFPS(float prevElapsed);

// BigRevision changes
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

	// FPS calculation
	int mFrameCount;
	float mFPSSamples[MAX_FPS_SAMPLES];

	// #TODO(Josh) Move this. Think about loop encapsulation and organization
	float mDeltaTime;

	// This should be replaced by a more robust and friendly API but will do for now.
	U32 mRenderSystemId;
	U32 mECSId;

	bool bIsInitialized;
	bool bShouldExit;
	bool bDisplayDebugServices;
};
