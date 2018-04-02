#pragma once

#include <RZE_Config.h>

#include <Utils/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/SubSystemHandler.h>
#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Game/GameScene.h>

#include <Apollo/EntityHandler.h>

#include <Diotima/Renderer.h>

class RZE_Application;

class GameWorld;
class Win32Window;

namespace Apollo
{
	class EntityComponentSystem;
}

class RZE_Engine
{
	friend class RZE_Editor;

public:

	RZE_Engine();
	~RZE_Engine();

	inline bool IsInitialized() { return bIsInitialized; }

	void Run(Functor<RZE_Application* const>& createApplicationCallback);

	const Vector2D& GetWindowSize() const;

	ResourceHandler&			GetResourceHandler();
	InputHandler&				GetInputHandler() { return mInputHandler; }
	Diotima::Renderer*			GetRenderer() const { return mRenderer; }
	GameScene&					GetActiveScene();


	// #TODO(Josh) this needs to return an actual thing, just placeholder atm
	inline double GetDeltaTime() const { return mDeltaTime; }
	
	void PostExit();

public:
	void Log(const std::string& text, const Vector3D& color);

private:

	void Init();
	void PostInit(Functor<RZE_Application* const>& createApplicationCallback);

	void PreUpdate(); // Set up anything the new frame will need
	void Update();

	void BeginShutDown();
	void InternalShutDown();

	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterEngineComponentTypes();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();

	void InitializeApplication(Functor<RZE_Application* const> createGameCallback);

private:
	RZE_Application* mApplication;
	GameScene* mActiveScene;

	Win32Window* mMainWindow;

	ResourceHandler mResourceHandler;
	EventHandler mEventHandler;
	InputHandler mInputHandler;

	Diotima::Renderer* mRenderer;

	EngineConfig* mEngineConfig;
	
	// PODs
private:
	double mDeltaTime	{ 0.0f };

	bool bIsInitialized;
	bool bShouldExit;
};
