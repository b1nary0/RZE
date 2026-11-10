#pragma once

#include <Utils/Platform/File.h>

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
class SplashScreen;
class Window;

class RZE_Engine
{
public:
	RZE_Engine();
	~RZE_Engine();

	bool IsInitialized() { return m_isInitialized; }

	void Run(Functor<RZE_Application* const>& createApplicationCallback);

	[[nodiscard]]
	const Vector2D& GetWindowSize() const;

	// The application the engine runs (Run). The application reaches the engine through RZE().
	RZE_Application&			GetApplication();
	ResourceHandler&			GetResourceHandler();
	InputHandler&				GetInputHandler();
	RenderEngine&				GetRenderEngine();
	GameScene&					GetActiveScene();
	AsyncOperationManager&		GetAsyncOperationManager();

	// When frames start and how long they get. Change its settings through SetFrameSettings.
	const FramePacer& GetFramePacer() const;
	// Frame rate caps and vsync, from Engine.ini's [FrameSettings] at start-up
	void SetFrameSettings(const FrameSettings& settings);

	// What this frame simulates: the time since the previous frame, clamped and evened out (FramePacer::GetDeltaTime)
	[[nodiscard]]
	double GetDeltaTime() const { return m_framePacer.GetDeltaTime(); }
	[[nodiscard]]
	float GetDeltaTimeMS() const { return static_cast<float>(GetDeltaTime() * 1000); }
	
	void PostExit();

public:
	void SetWindowSize(const Vector2D& newSize);

	const Vector2D& GetWindowClientSize() const;

private:

	void Init();
	void PostInit();
	// Ticks async operations, with no frames, until the application is ready to show (RZE_Application::IsReadyToShow)
	void WaitForStartupLoading();
	// Draws the first frames while the window is still cloaked, and waits for the last to be presented, so it shows
	// finished rather than blank or half built
	void DrawFirstFrames();

	// Reads [FrameSettings] and the -vsync, -maxfps and -uifps arguments
	void InitializeFramePacing();
	// Top of each frame, once the wait for it is over: the frame's start time and delta time
	void BeginFrame(double now);
	// A frame's work, from input to handing it to the renderer, for the main loop and DrawFirstFrames. Returns how long
	// the update took in milliseconds, for the frame time capture.
	double RunFrame(double frameStart);
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
	// Engine.ini's LaunchMonitor, or -launchMonitor's
	int GetLaunchMonitor();
	// The application's window title, or else the engine config's
	std::string GetWindowTitle();
	// Shows what start-up is doing on the splash screen
	void SetStartupStatus(const std::string& status);
	// With progress from 0 to 1
	void SetStartupStatus(const std::string& status, float progress);
	void CreateAndInitializeWindow();
	void CreateAndInitializeRenderer();

	void InitializeApplication();

private:
	// Not owned: the application whose Run() started the engine (a local in each main). It outlives Run, so it's still
	// there when ImGui's context is destroyed after the application's ShutDown, and saves imgui.ini through what the
	// application gave it (the editor's ini path and settings handlers).
	RZE_Application* m_application;
	GameScene* m_activeScene;

	Window* m_window;
	// From once Engine.ini is loaded until start-up is done, if the application has a splash screen
	// (RZE_Application::GetSplashScreenImage)
	std::unique_ptr<SplashScreen> m_splashScreen;

	ResourceHandler m_resourceHandler;
	EventHandler m_eventHandler;
	InputHandler m_inputHandler;
	AsyncOperationManager m_asyncOperationManager;

	std::unique_ptr<RenderEngine> m_renderEngine;

	ResourceHandle m_engineConfig;
	
	// PODs
private:
	bool m_isInitialized;
	bool m_shouldExit;

	// Started when the main loop starts. Frame times are measured on it.
	HiResTimer m_programTimer;
	FramePacer m_framePacer;
	PreciseSleep m_frameSleep;

	FrameTimeCapture m_frameTimeCapture;
};
