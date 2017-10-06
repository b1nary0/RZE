#pragma once

#include <EngineApp.h>

#include <DebugUtils/DebugServices.h>

#include <EngineCore/Platform/File.h>

#include <EngineUI/Windows/LogWindow.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>
#include <RenderCore/Graphics/Font/FontHandler.h>

#include <RenderCore/SceneCamera.h>

class EditorEnvironment;
class GameWorld;
class RZE_Renderer;
class Win32Window;

class RZE_Engine
{
	static RZE_Engine* sInstance;

public:

	RZE_Engine();
	~RZE_Engine();

	void Run(Functor<RZE_Game* const>& createApplicationCallback);

	void RegisterForEvent(const U16 eventType, Functor<void, const Event&> callback);
	void RegisterForInputEvent(const U16 eventType, Functor<void, U8> callback);

	// #TODO(Josh) This should be revisited when the camera access/design changes
	SceneCamera&		GetSceneCamera();
	WindowSettings&		GetWindowSettings();
	GameWorld&			GetWorld() const;
	ResourceHandler&	GetResourceHandler();
	FontHandler&		GetFontHandler();

	void SetCursorEnabled(bool enabled);

	static RZE_Engine* const Get()
	{
		if (!sInstance)
		{
			sInstance = new RZE_Engine();
		}

		return sInstance;
	}

private:

	// #TODO(Josh) Compartmentalize this later.
	void InitImGUI();

	void Init();
	void InitWorld();
	void PostInit(Functor<RZE_Game* const>& createApplicationCallback);

	void Update();

	void BeginShutDown();
	void InternalShutDown();
	void PostExit();

	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterInputEvents();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();

	void LoadFonts();

	void InitGame(Functor<RZE_Game* const> createGameCallback);
		
	RZE_Game* mApplication;
	GameWorld* mWorld;
	EditorEnvironment* mEditorEnv;

	Win32Window* mMainWindow;
	RZE_Renderer* mRenderer;

	ResourceHandler mResourceHandler;
	EventHandler mEventHandler;
	InputHandler mInputHandler;
	FontHandler mFontHandler;

	EngineConfig* mEngineConfig;

	bool bIsInitialized;
	bool bShouldExit;
};
