#pragma once

#include <EngineApp.h>

#include <RZE_Config.h>

#include <EngineCore/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/SubSystemHandler.h>
#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>
#include <RenderCore/Graphics/Font/FontHandler.h>

#include <RenderCore/SceneCamera.h>

class GameWorld;
class RZE_Renderer;
class Win32Window;

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

	// #TODO(Josh) This should be revisited when the camera access/design changes
	SceneCamera&		GetSceneCamera();
	WindowSettings&		GetWindowSettings();
	GameWorld&			GetWorld() const;
	ResourceHandler&	GetResourceHandler();
	FontHandler&		GetFontHandler();

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

	void LoadFonts();

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
	RZE_Renderer* mRenderer;

	ResourceHandler mResourceHandler;
	EventHandler mEventHandler;
	InputHandler mInputHandler;
	FontHandler mFontHandler;

	EngineConfig* mEngineConfig;

	// FPS calculation
	int mFrameCount;
	float mFPSSamples[MAX_FPS_SAMPLES];

	// #TODO(Josh) Move this. Think about loop encapsulation and organization
	float mDeltaTime;

	bool bIsInitialized;
	bool bShouldExit;
	bool bDisplayDebugServices;
};
