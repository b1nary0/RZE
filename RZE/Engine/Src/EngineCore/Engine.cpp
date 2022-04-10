#include <StdAfx.h>
#include <EngineCore/Engine.h>

#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <DebugUtils/DebugServices.h>

#include <Graphics/RenderEngine.h>

#include <Windowing/Win32Window.h>

#include <Utils/DebugUtils/Debug.h>

#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>
#include <imGUI/ImGuizmo.h>

#include "Windowing/Win32Window.h"

RZE_Engine::RZE_Engine()
	: m_window(nullptr)
	, m_engineConfig(nullptr)
	, m_application(nullptr)
	, m_renderEngine(nullptr)
	, m_shouldExit(false)
	, m_isInitialized(false)
	, m_deltaTime(0.0)
	, m_frameCount(0)
{
	m_frameSamples.resize(MAX_FRAMETIME_SAMPLES);
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

		HiResTimer programTimer;
		programTimer.Start();
		double prevTime = programTimer.GetElapsed<double>();
		while (!m_shouldExit)
		{
			OPTICK_FRAME("Main");

			double currTime = programTimer.GetElapsed<double>();
			double frameTime = currTime - prevTime;
			prevTime = currTime;

			m_deltaTime = frameTime;
			m_frameSamples[m_frameCount % MAX_FRAMETIME_SAMPLES] = static_cast<float>(m_deltaTime);

			const float averageFrametime = CalculateAverageFrametime();

			static float frameTimeBuffer[MAX_FRAMETIME_SAMPLES];
			for (int idx = 0; idx < MAX_FRAMETIME_SAMPLES; ++idx)
			{
				frameTimeBuffer[idx] = m_frameSamples[idx] * 1000.0f;
			}

			{	
				OPTICK_EVENT("RZE_Engine::Run");

				PreUpdate();
				{
					OPTICK_EVENT("Update and Render");
					
					{
						OPTICK_EVENT("ImGui Frame Work");

						ImGui_ImplDX11_NewFrame();
						ImGui_ImplWin32_NewFrame();
						ImGui::NewFrame();
						ImGuizmo::BeginFrame();
					}

					Update();

					m_renderEngine->Update();
					
					ImGui::EndFrame();

					m_renderEngine->Render();
				}
			}

			++m_frameCount;
		}

		BeginShutDown();
	}
	else
	{
		LOG_CONSOLE("Initialization Failed.");
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
		LOG_CONSOLE("RZE_EngineCore::Init() called.");

		DebugServices::Get().Initialize();

		Threading::JobScheduler::Get().Initialize();

		LoadEngineConfig();

		CreateAndInitializeWindow();

		m_inputHandler.Initialize();

		RegisterWindowEvents();

		m_resourceHandler.Init();

		CreateAndInitializeRenderer();

		m_activeScene = new GameScene();
		m_activeScene->Initialize();

		m_isInitialized = true;
	}
}

void RZE_Engine::PostInit(Functor<RZE_Application* const>& createApplicationCallback)
{
	LOG_CONSOLE("RZE_EngineCore::PostInit() called.");

	RegisterKeyEvents();

	InitializeApplication(createApplicationCallback);

	m_activeScene->Start();
}

void RZE_Engine::PreUpdate()
{	
	OPTICK_EVENT();

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
	WindowSettings& windowSettings = m_engineConfig->GetWindowSettings();

	Win32Window::WindowCreationParams params;
	params.windowTitle = windowSettings.GetTitle();
	params.width = static_cast<int>(windowSettings.GetDimensions().X());
	params.height = static_cast<int>(windowSettings.GetDimensions().Y());

	m_window = new Win32Window(params);
	AssertNotNull(m_window);

	m_window->RegisterEvents(m_eventHandler);

	m_window->Show();
}

void RZE_Engine::CreateAndInitializeRenderer()
{
	LOG_CONSOLE("Initializing Renderer");

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

float RZE_Engine::CalculateAverageFrametime()
{
	float sum = 0.0f;
	for (float frameSample : m_frameSamples)
	{
		sum += frameSample;
	}

	sum /= MAX_FRAMETIME_SAMPLES;
	return sum;
}


void RZE_Engine::SetWindowSize(Vector2D newSize)
{
	AssertNotNull(m_window);
	m_window->SetWindowSize(newSize);
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
		else if (event.mWindowEvent.mEventInfo.mEventSubType == EWindowEventType::Window_Resize)
		{
			Vector2D newSize(event.mWindowEvent.mSizeX, event.mWindowEvent.mSizeY);

			m_application->OnWindowResize(newSize);
			m_renderEngine->ResizeCanvas(newSize);
		}
	});
	m_eventHandler.RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::RegisterKeyEvents()
{
 	ImGuiIO& io = ImGui::GetIO();
 	io.KeyMap[ImGuiKey_Enter] = Win32KeyCode::Return;
}

void RZE_Engine::LoadEngineConfig()
{
	// #TODO(Josh) This should probably be a resource? 
	m_engineConfig = new EngineConfig();
	m_engineConfig->Load(Filepath("Config/Engine.ini"));

	if (m_engineConfig->Empty())
	{
		LOG_CONSOLE("Engine config could not load. Defaults were used.");
	}
}

void RZE_Engine::Update()
{
	OPTICK_EVENT();

	m_application->Update();
	m_activeScene->Update();
}

void RZE_Engine::BeginShutDown()
{
	LOG_CONSOLE("Shutting engine down...");
	
	m_activeScene->ShutDown();
	m_application->ShutDown();
	m_renderEngine->Shutdown();

	m_resourceHandler.ShutDown();
	Threading::JobScheduler::Get().ShutDown();

	InternalShutDown();
}

void RZE_Engine::InternalShutDown()
{
	AssertNotNull(m_window);
	AssertNotNull(m_engineConfig);
	AssertNotNull(m_renderEngine);
	AssertNotNull(m_activeScene);

	delete m_activeScene;
	m_renderEngine.reset();
	delete m_window;
	delete m_engineConfig;
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

bool RZE_Engine::ShowOpenFilePrompt(const FilePromptParams& params, std::string& chosenPath)
{
	AssertNotNull(m_window);
	return m_window->ShowOpenFilePrompt(params, chosenPath);
}

void RZE_Engine::Log(const std::string& text, const Vector3D& color)
{
}

