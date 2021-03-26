#pragma once

#include <RZE_Config.h>

#include <Utils/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <Game/GameScene.h>

#include <Apollo/EntityHandler.h>

#include <Diotima/LegacyRenderer.h>
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
public:
	RZE_Engine();
	~RZE_Engine();

	inline bool IsInitialized() { return bIsInitialized; }

	void Run(Functor<RZE_Application* const>& createApplicationCallback);

	const Vector2D& GetWindowSize() const;

	// #TODO(Josh) Need to figure out this weird connection of accessing engine from application, and application from engine.
	RZE_Application&			GetApplication();
	// #TODO
	// All of these gets might be best to hide behind a cpp. Or static engine in CPP memory with accessor in h.
	ResourceHandler&			GetResourceHandler();
	InputHandler&				GetInputHandler();
	Diotima::Renderer&			GetRenderer();
	GameScene&					GetActiveScene();

	// #TODO(Should probably put the window in the app..)
	FilePath ShowOpenFilePrompt();

	inline double GetDeltaTime() const { return mDeltaTime; }
	
	void PostExit();

public:
	void Log(const std::string& text, const Vector3D& color);

	void SetWindowSize(Vector2D newSize);

private:

	void Init();
	void PostInit(Functor<RZE_Application* const>& createApplicationCallback);

	void PreUpdate(); // Set up anything the new frame will need
	void Update();

	void BeginShutDown();
	void InternalShutDown();

	void CompileEvents();

	void RegisterWindowEvents();
	void RegisterKeyEvents();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();
	void CreateAndInitializeRenderer();

	void InitializeApplication(Functor<RZE_Application* const> createGameCallback);

	float CalculateAverageFrametime();

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
	double mDeltaTime;
	U64 mFrameCount;

	bool bIsInitialized;
	bool bShouldExit;

	std::vector<float> mFrameSamples;
};
