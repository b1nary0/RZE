#include <StdAfx.h>
#include <EngineCore/Engine.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <DebugUtils/DebugServices.h>

#include <Graphics/RenderEngine.h>

#include <Windowing/Display.h>
#include <Windowing/Win32Window.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/CmdLine.h>

#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>
#include <imGUI/ImGuizmo.h>

#include "Windowing/Win32Window.h"

namespace
{
	// Less spare time than this isn't worth handing to async operations
	constexpr double k_minIdleWorkMS = 1.0;

	// Forwards RZE_LOG/RZE_LOG_ARGS lines to DebugServices (the editor Log panel). Worker-thread lines are
	// marshalled to the main thread, since DebugServices must not be traced while it is being iterated.
	void TraceLogLine(std::string_view line)
	{
		if (Threading::MainThreadDispatcher::Get().IsMainThread())
		{
			DebugServices::Get().Trace(LogChannel::Info, std::string(line));
			return;
		}

		Threading::MainThreadDispatcher::Get().Post(Threading::Job::Task([text = std::string(line)]()
			{
				DebugServices::Get().Trace(LogChannel::Info, text);
			}));
	}
}

RZE_Engine::RZE_Engine()
	: m_window(nullptr)
	, m_application(nullptr)
	, m_renderEngine(nullptr)
	, m_shouldExit(false)
	, m_isInitialized(false)
	, m_deltaTime(0.0)
	, m_frameCount(0)
{
}

RZE_Engine::~RZE_Engine()
{
}

void RZE_Engine::Run(Functor<RZE_Application* const>& createApplicationCallback)
{
	Init();

	if (m_isInitialized)
	{
		AssertNotNull(createApplicationCallback);
		PostInit(createApplicationCallback);

		m_programTimer.Start();

		m_frameTimeCapture.Initialize();
		// After the capture, which turns pacing off
		InitializeFramePacing();

		double lastUpdateMS = 0.0;

		while (!m_shouldExit)
		{
			OPTICK_FRAME("Main");

			const double frameStart = m_programTimer.GetElapsed<double>();
			BeginFrame(frameStart);

			if (m_frameTimeCapture.AddFrame(frameStart, m_framePacer.GetFrameTime() * 1000.0, lastUpdateMS))
			{
				break;
			}

			{	
				OPTICK_EVENT("RZE_Engine::Run");

				PreUpdate();
				{
					OPTICK_EVENT("Update and Render");

					// The frame capture's update time covers the async tick, the application's update and UI, and the scene's
					// update, but not ImGui's NewFrame
					double updateStart = m_programTimer.GetElapsed<double>();
					{
						OPTICK_EVENT("AsyncOperationManager::Tick");
						// First, so completion callbacks (e.g. a scene finishing loading) land before the application gathers
						// this frame's state, rather than between that and the UI being drawn from it
						m_asyncOperationManager.Tick();
					}
					double updateMS = (m_programTimer.GetElapsed<double>() - updateStart) * 1000.0;

					updateMS += UpdateUI();

					updateStart = m_programTimer.GetElapsed<double>();
					Update();
					lastUpdateMS = updateMS + (m_programTimer.GetElapsed<double>() - updateStart) * 1000.0;

					m_renderEngine->Render("Main Render");
					m_renderEngine->Finish();

					// After the present, which is what lets the render thread's frame catch up
					m_resourceHandler.Update();
				}
			}

			WaitForNextFrame();

			++m_frameCount;
		}

		BeginShutDown();
	}
	else
	{
		RZE_LOG("Initialization Failed.");
	}
}

const Vector2D& RZE_Engine::GetWindowSize() const
{
	AssertNotNull(m_window);
	return m_window->GetDimensions();
}

RZE_Application& RZE_Engine::GetApplication()
{
	AssertNotNull(m_application);
	return *m_application;
}

void RZE_Engine::Init()
{
	if (!IsInitialized())
	{
		RZE_LOG("RZE_EngineCore::Init() called.");

		Debug::CreateLogFile();

		DebugServices::Get().Initialize();

		Threading::MainThreadDispatcher::Get().Initialize();
		Debug::SetLogSink(&TraceLogLine);
		Threading::JobScheduler::Get().Initialize();
		m_asyncOperationManager.Initialize();

		LoadEngineConfig();

		CreateAndInitializeWindow();

		m_inputHandler.Initialize();

		RegisterWindowEvents();

		m_resourceHandler.Init();

		CreateAndInitializeRenderer();

		m_activeScene = new GameScene();
		m_activeScene->Initialize();
		m_renderEngine->SetMainScene(m_activeScene->GetRenderScene());

		m_isInitialized = true;
	}
}

void RZE_Engine::PostInit(Functor<RZE_Application* const>& createApplicationCallback)
{
	RZE_LOG("RZE_EngineCore::PostInit() called.");

	InitializeApplication(createApplicationCallback);

	m_activeScene->Start();
}

void RZE_Engine::PreUpdate()
{	
	OPTICK_EVENT();

	Threading::MainThreadDispatcher::Get().Drain();

	CompileEvents();
	m_eventHandler.ProcessEvents();

	if (m_application->ProcessInput(m_inputHandler))
	{
		m_inputHandler.RaiseEvents();
	}
	else
	{
		m_inputHandler.Reset();
	} 
}

void RZE_Engine::CreateAndInitializeWindow()
{
	const WindowSettings& windowSettings = GetResourceHandler().GetResource<EngineConfig>(m_engineConfig)->GetWindowSettings();

	Win32Window::WindowCreationParams params;
	params.windowTitle = windowSettings.GetTitle();
	params.width = static_cast<int>(windowSettings.GetDimensions().X());
	params.height = static_cast<int>(windowSettings.GetDimensions().Y());

	// Command line overrides the config value
	params.monitorIndex = windowSettings.GetLaunchMonitor();
	CmdLine::Arguments::GetInt("-launchMonitor", params.monitorIndex);

	m_window = new Win32Window(params);
	AssertNotNull(m_window);

	m_window->RegisterEvents(m_eventHandler);

	m_window->Show();
}

void RZE_Engine::CreateAndInitializeRenderer()
{
	RZE_LOG("Initializing Renderer");

	m_renderEngine = std::make_unique<RenderEngine>();
	m_renderEngine->Initialize(m_window->GetOSWindowHandleData().windowHandle);
}

void RZE_Engine::InitializeApplication(Functor<RZE_Application* const> createGameCallback)
{
	m_application = createGameCallback();
	AssertNotNull(m_application);

	m_application->SetWindow(m_window);
	m_application->Initialize();
	m_application->RegisterInputEvents(m_inputHandler);

	// #IDEA
	// Move this out and controlled in another manner. At this point
	// we instead just initialize the application,and start it much later 
	// explicitly in RZE_Engine::Initialize or something
	m_application->Start();
}

void RZE_Engine::InitializeFramePacing()
{
	FrameSettings settings = GetResourceHandler().GetResource<EngineConfig>(m_engineConfig)->GetFrameSettings();

	// A capture measures what frames cost, comparable with captures from before frames were paced, so it runs
	// unpaced unless the command line asks for pacing too
	if (m_frameTimeCapture.IsEnabled())
	{
		settings.VSync = false;
		settings.MaxFrameRate = 0.0;
		settings.UnfocusedFrameRate = 0.0;
	}

	int vsync = 0;
	if (CmdLine::Arguments::GetInt("-vsync", vsync))
	{
		settings.VSync = vsync != 0;
	}

	int maxFrameRate = 0;
	if (CmdLine::Arguments::GetInt("-maxfps", maxFrameRate))
	{
		settings.MaxFrameRate = static_cast<double>(maxFrameRate);
	}

	m_framePacer.SetRefreshRate(Display::GetRefreshRate(m_window->GetOSWindowHandleData().windowHandle));
	SetFrameSettings(settings);
}

void RZE_Engine::BeginFrame(double now)
{
	m_framePacer.SetIsThrottled(!m_window->IsFocused() || m_window->IsMinimised());
	m_framePacer.BeginFrame(now);

	m_deltaTime = m_framePacer.GetDeltaTime();
}

void RZE_Engine::WaitForNextFrame()
{
	OPTICK_EVENT();

	const double workEnd = m_programTimer.GetElapsed<double>();
	m_framePacer.EndFrameWork(workEnd);

	// Spare time before the next frame goes to async operations (scene loads, thumbnails and the like). Only frames that
	// are paced have any; it's after Finish(), so it adds nothing to this frame's latency.
	const double idleMS = m_framePacer.GetIdleTime(workEnd) * 1000.0;
	if (idleMS >= k_minIdleWorkMS)
	{
		OPTICK_EVENT("Idle work");
		m_asyncOperationManager.TickIdle(idleMS);
		m_framePacer.AddIdleWork(m_programTimer.GetElapsed<double>() - workEnd);
	}

	{
		OPTICK_EVENT("Sleep");
		m_frameSleep.SleepFor(m_framePacer.GetNextFrameStart() - m_programTimer.GetElapsed<double>());
	}

	// Last, so input is read as late as possible: with vsync this is what paces frames, and either way it keeps no
	// more than two frames queued
	m_renderEngine->WaitForPresentSlot();
}

void RZE_Engine::SetFrameSettings(const FrameSettings& settings)
{
	m_framePacer.SetSettings(settings);
	m_renderEngine->SetVSync(settings.VSync);
}

const FramePacer& RZE_Engine::GetFramePacer() const
{
	return m_framePacer;
}


void RZE_Engine::SetWindowSize(const Vector2D& newSize)
{
	AssertNotNull(m_window);
	m_window->SetWindowSize(newSize);
}

const Vector2D& RZE_Engine::GetWindowClientSize() const
{
	return m_window->GetClientSize();
}

void RZE_Engine::CompileEvents()
{
	m_window->CompileInputMessages(m_inputHandler);
	m_window->CompileWindowMessages(m_eventHandler);
}

void RZE_Engine::RegisterWindowEvents()
{
	Functor<void, const Event&> windowCallback([this](const Event& event)
	{
		AssertEqual(event.mInfo.mEventType, EEventType::Window);
		if (event.mWindowEvent.mEventInfo.mEventSubType == EWindowEventType::Window_Destroy)
		{
			PostExit();
		}
		else if (event.mWindowEvent.mEventInfo.mEventSubType == EWindowEventType::Window_CloseRequested)
		{
			if (m_application->OnCloseRequested())
			{
				PostExit();
			}
		}
		else if (event.mWindowEvent.mEventInfo.mEventSubType == EWindowEventType::Window_Resize)
		{
			Vector2D newSize(event.mWindowEvent.mSizeX, event.mWindowEvent.mSizeY);

			m_application->OnWindowResize(newSize);

			m_renderEngine->ResizeCanvas(newSize);

			// Also sent after a move, which may have taken the window to a monitor with another refresh rate
			m_framePacer.SetRefreshRate(Display::GetRefreshRate(m_window->GetOSWindowHandleData().windowHandle));
		}
	});
	m_eventHandler.RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::LoadEngineConfig()
{
	m_engineConfig = GetResourceHandler().LoadResource<EngineConfig>(Filepath("Config/Engine.ini"));
	const EngineConfig* engineConfig = GetResourceHandler().GetResource<EngineConfig>(m_engineConfig);
	if (engineConfig->Empty())
	{
		RZE_LOG("Engine config could not load. Defaults were used.");
	}
}

double RZE_Engine::UpdateUI()
{
	OPTICK_EVENT();

	double start = m_programTimer.GetElapsed<double>();
	m_application->PreUIUpdate();
	double applicationMS = (m_programTimer.GetElapsed<double>() - start) * 1000.0;

	{
		OPTICK_EVENT("ImGui NewFrame Work");

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	start = m_programTimer.GetElapsed<double>();
	m_application->UpdateUI();
	applicationMS += (m_programTimer.GetElapsed<double>() - start) * 1000.0;

	ImGui::EndFrame();

	return applicationMS;
}

void RZE_Engine::Update()
{
	OPTICK_EVENT();

	m_application->Update();
	m_activeScene->Update();
	m_application->PreRender();
}

void RZE_Engine::BeginShutDown()
{
	RZE_LOG("Shutting engine down...");

	// First, so no async work is touching the scene, resources or renderer while they shut down.
	m_asyncOperationManager.ShutDown();

	m_activeScene->ShutDown();
	m_application->ShutDown();

	//@todo this is a quick fix for the cyclical dependency between Rendering::MemArena and ResourceHandler releasing leftover reasources.
	m_renderEngine->GetMainScene().ClearObjects();
	m_renderEngine->ReleaseRenderStages();

	// What was released while shutting down is waiting on the render thread (ResourceHandler::DestroyWhenRendered). It
	// goes once the render thread has finished its last frame, and before the renderer shuts down, since releasing a
	// GPU resource records a render command.
	m_renderEngine->WaitUntilIdle();
	m_resourceHandler.DestroyPendingNow();

	m_renderEngine->Shutdown();

	Threading::JobScheduler::Get().ShutDown();
	Debug::SetLogSink(nullptr);
	Threading::MainThreadDispatcher::Get().ShutDown();

	InternalShutDown();
}

void RZE_Engine::InternalShutDown()
{
	AssertNotNull(m_window);
	AssertNotNull(m_renderEngine);
	AssertNotNull(m_activeScene);

	delete m_activeScene;
	m_renderEngine.reset();

	m_engineConfig = ResourceHandle::EmptyHandle();
	m_resourceHandler.ShutDown();
	delete m_window;
}

void RZE_Engine::PostExit()
{
	m_shouldExit = true;
}

ResourceHandler& RZE_Engine::GetResourceHandler()
{
	return m_resourceHandler;
}

InputHandler& RZE_Engine::GetInputHandler()
{
	return m_inputHandler;
}

RenderEngine& RZE_Engine::GetRenderEngine()
{
	return *m_renderEngine.get();
}

GameScene& RZE_Engine::GetActiveScene()
{
	AssertNotNull(m_activeScene);
	return *m_activeScene;
}

AsyncOperationManager& RZE_Engine::GetAsyncOperationManager()
{
	return m_asyncOperationManager;
}

void RZE_Engine::Log(const std::string& text, const Vector3D& color)
{
}

