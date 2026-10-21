#pragma once

#include <RZE_Config.h>

#include <Utils/Platform/File.h>

// @todo:josh this is where the weird include-before issue is
#include <Events/EventHandler.h>

#include <DebugUtils/FrameTimeCapture.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Config/EngineConfig.h>
#include <EngineCore/Input/InputHandler.h>
#include <EngineCore/Resources/ResourceHandler.h>
#include <EngineCore/Timing/FramePacer.h>

#include <Game/GameScene.h>

#include <Utils/Platform/Timers/HiResTimer.h>
#include <Utils/Platform/Timers/PreciseSleep.h>

class RZE_Application;

class RenderEngine;
class Win32Window;

class RZE_Engine
{
public:
	RZE_Engine();
	~RZE_Engine();

	bool IsInitialized() { return m_isInitialized; }

	void Run(Functor<RZE_Application* const>& createApplicationCallback);

	[[nodiscard]]
	const Vector2D& GetWindowSize() const;

	// #TODO(Josh) Need to figure out this weird connection of accessing engine from application, and application from engine.
	RZE_Application&			GetApplication();
	// #TODO
	// All of these gets might be best to hide behind a cpp. Or static engine in CPP memory with accessor in h.
	ResourceHandler&			GetResourceHandler();
	InputHandler&				GetInputHandler();
	RenderEngine&			GetRenderEngine();
	GameScene&					GetActiveScene();
	AsyncOperationManager&		GetAsyncOperationManager();

	// When frames start and how long they get. Change its settings through SetFrameSettings.
	const FramePacer& GetFramePacer() const;
	// Frame rate caps and vsync, from Engine.ini's [FrameSettings] at start-up
	void SetFrameSettings(const FrameSettings& settings);

	// What this frame simulates: the time since the previous frame, clamped and evened out (FramePacer::GetDeltaTime)
	[[nodiscard]]
	double GetDeltaTime() const { return m_deltaTime; }
	[[nodiscard]]
	float GetDeltaTimeMS() const { return static_cast<float>(m_deltaTime * 1000); }
	
	void PostExit();

public:
	void Log(const std::string& text, const Vector3D& color);

	void SetWindowSize(const Vector2D& newSize);

	const Vector2D& GetWindowClientSize() const;

private:

	void Init();
	void PostInit(Functor<RZE_Application* const>& createApplicationCallback);

	// Reads [FrameSettings] and the -vsync, -maxfps and -uifps arguments
	void InitializeFramePacing();
	// Top of each frame, once the wait for it is over: the frame's start time and delta time
	void BeginFrame(double now);
	// After the frame is handed to the renderer: waits until the next frame is due
	void WaitForNextFrame();

	void PreUpdate(); // Set up anything the new frame will need
	// The UI's part of the frame: the application's PreUIUpdate, then its UpdateUI inside ImGui's frame. Returns how
	// long the application's two calls took in milliseconds, which is update time for the frame time capture.
	double UpdateUI();
	// The rest of the frame's update, outside ImGui's frame: the application's Update, the scene's update, then the
	// application's PreRender
	void Update();

	void BeginShutDown();
	void InternalShutDown();
	
	void CompileEvents();

	void RegisterWindowEvents();

	void LoadEngineConfig();
	void CreateAndInitializeWindow();
	void CreateAndInitializeRenderer();

	void InitializeApplication(Functor<RZE_Application* const> createGameCallback);

private:
	// Never deleted. ImGui's context is destroyed after the application's ShutDown, and saves imgui.ini through what the
	// application gave it (the editor's ini path and settings handlers), so deleting it would have to come after that.
	RZE_Application* m_application;
	GameScene* m_activeScene;

	Win32Window* m_window;

	ResourceHandler m_resourceHandler;
	EventHandler m_eventHandler;
	InputHandler m_inputHandler;
	AsyncOperationManager m_asyncOperationManager;

	std::unique_ptr<RenderEngine> m_renderEngine;

	ResourceHandle m_engineConfig;
	
	// PODs
private:
	double m_deltaTime;
	U64 m_frameCount;

	bool m_isInitialized;
	bool m_shouldExit;

	// Started when the main loop starts. Frame times are measured on it.
	HiResTimer m_programTimer;
	FramePacer m_framePacer;
	PreciseSleep m_frameSleep;

	FrameTimeCapture m_frameTimeCapture;
};
