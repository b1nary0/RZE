#include <StdAfx.h>
#include <EngineCore/Engine.h>

#include <DebugUtils/DebugServices.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <ECS/Systems/RenderSystem.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

#include <Utils/DebugUtils/Debug.h>

#include <Diotima/Renderer.h>

RZE_Engine::RZE_Engine()
{
	mMainWindow = nullptr;
	mEngineConfig = nullptr;
	mApplication = nullptr;
	mRenderer = nullptr;

	bShouldExit = false;
	bIsInitialized = false;
}

RZE_Engine::~RZE_Engine()
{
}

void RZE_Engine::Run(Functor<RZE_Application* const>& createApplicationCallback)
{
	Init();

	if (bIsInitialized)
	{
		AssertNotNull(createApplicationCallback);
		PostInit(createApplicationCallback);

		HiResTimer programTimer;
		programTimer.Start();
		double prevTime = programTimer.GetElapsed<double>();
		while (!bShouldExit)
		{
			BROFILER_FRAME("Engine Thread");

			double currTime = programTimer.GetElapsed<double>();
			double frameTime = currTime - prevTime;
			prevTime = currTime;

			mDeltaTime = frameTime;

			{	BROFILER_CATEGORY("RZE_Engine::Run", Profiler::Color::Cyan)
				// #TODO(Josh) Need to work this out but for the moment we need to pre update and then start the imgui new frame for things like editor stealing imgui input etc
				PreUpdate();

				DebugServices::AddData(StringUtils::FormatString("Frame Time: %f ms", static_cast<float>(mDeltaTime) * 1000.0f), Vector3D(1.0f, 1.0f, 0.0f));
				{
					Update();

					DebugServices::Display(GetWindowSize());

					mRenderer->Update();
					ImGui::Render();
				}
			}

			mMainWindow->BufferSwap(); // #TODO(Josh) Maybe this can be done better
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
	AssertNotNull(mMainWindow);
	return mMainWindow->GetDimensions();
}

RZE_Application& RZE_Engine::GetApplication()
{
	AssertNotNull(mApplication);
	return *mApplication;
}

void RZE_Engine::Init()
{
	if (!IsInitialized())
	{
		LOG_CONSOLE("RZE_EngineCore::Init() called.");

		Perseus::JobScheduler::Get().Initialize();

		LoadEngineConfig();

		CreateAndInitializeWindow();

		mInputHandler.Initialize();

		RegisterWindowEvents();
		RegisterEngineComponentTypes();

		mResourceHandler.Init();

		CreateAndInitializeRenderer();

		DebugServices::Initialize();
		DebugServices::HandleScreenResize(GetWindowSize());

		mActiveScene = new GameScene();
		mActiveScene->Initialize();

		bIsInitialized = true;
	}
}

void RZE_Engine::PostInit(Functor<RZE_Application* const>& createApplicationCallback)
{
	LOG_CONSOLE("RZE_EngineCore::PostInit() called.");

	InitializeApplication(createApplicationCallback);

	mActiveScene->Start();
}

void RZE_Engine::PreUpdate()
{	BROFILER_CATEGORY("RZE_Engine::PreUpdate", Profiler::Color::Purple)
	CompileEvents();
	mEventHandler.ProcessEvents();

	if (mApplication->ProcessInput(mInputHandler))
	{
		mInputHandler.RaiseEvents();
	}
	else
	{
		mInputHandler.Reset();
	}

	// #NOTE(Josh) This has to do with the lack of confidence in the placement of ImGUI and its role.
	//				Should always happen at the end of the pre-update phase for now. Also want this here as
	//				it's currently considered part of the pre-update profile.
	ImGui::NewFrame();
}

void RZE_Engine::CreateAndInitializeWindow()
{
	WindowSettings& windowSettings = mEngineConfig->GetWindowSettings();

	Win32Window::WindowCreationParams params;
	params.windowTitle = windowSettings.GetTitle();
	params.width = static_cast<int>(windowSettings.GetDimensions().X());
	params.height = static_cast<int>(windowSettings.GetDimensions().Y());

	mMainWindow = new Win32Window(params);
	AssertNotNull(mMainWindow);

	mMainWindow->RegisterEvents(mEventHandler);

	mMainWindow->Show();
}

void RZE_Engine::CreateAndInitializeRenderer()
{
	mRenderer = new Diotima::Renderer();

	mRenderer->Initialize();
	mRenderer->EnableVsync(mEngineConfig->GetEngineSettings().IsVSyncEnabled());
}

void RZE_Engine::InitializeApplication(Functor<RZE_Application* const> createGameCallback)
{
	mApplication = createGameCallback();
	AssertNotNull(mApplication);

	mApplication->Initialize();
	mApplication->RegisterInputEvents(mInputHandler);
	mApplication->SetWindow(mMainWindow);

	const Vector2D& windowDims = mEngineConfig->GetWindowSettings().GetDimensions();
	mApplication->GetRenderTarget().SetWidth(static_cast<int>(windowDims.X()));
	mApplication->GetRenderTarget().SetHeight(static_cast<int>(windowDims.Y()));

	if (mApplication->IsEditor())
	{
		mRenderer->SetRenderTarget(&mApplication->GetRenderTarget());
	}

	// #TODO(Josh) Investigate a better transfer point than this re: render target setting

	mApplication->Start();
}

void RZE_Engine::CompileEvents()
{
	mMainWindow->CompileInputMessages(mInputHandler);
	mMainWindow->CompileWindowMessages(mEventHandler);
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
			GetRenderer().ResizeCanvas(newSize);
			DebugServices::HandleScreenResize(newSize);
		}
	});
	mEventHandler.RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::RegisterEngineComponentTypes()
{
	APOLLO_REGISTER_COMPONENT(CameraComponent);
	APOLLO_REGISTER_COMPONENT(LightSourceComponent);
	APOLLO_REGISTER_COMPONENT(MaterialComponent);
	APOLLO_REGISTER_COMPONENT(MeshComponent);
	APOLLO_REGISTER_COMPONENT(NameComponent);
	APOLLO_REGISTER_COMPONENT(TransformComponent);
}

void RZE_Engine::LoadEngineConfig()
{
	// #TODO(Josh) This should probably be a resource? 
	mEngineConfig = new EngineConfig();
	mEngineConfig->Load(FilePath("Engine/RZE/Config/Engine.ini"));

	if (mEngineConfig->Empty())
	{
		LOG_CONSOLE("Engine config could not load. Defaults were used.");
	}
}

void RZE_Engine::Update()
{	BROFILER_CATEGORY("RZE_Engine::Update", Profiler::Color::Orchid)
	mActiveScene->Update();
	mApplication->Update();
}

void RZE_Engine::BeginShutDown()
{
	LOG_CONSOLE("Shutting engine down...");
	
	mActiveScene->Finish();
	mApplication->ShutDown();
	mResourceHandler.ShutDown();

	Perseus::JobScheduler::Get().ShutDown();

	InternalShutDown();
}

void RZE_Engine::InternalShutDown()
{
	AssertNotNull(mMainWindow);
	AssertNotNull(mEngineConfig);
	AssertNotNull(mRenderer);
	AssertNotNull(mActiveScene);

	delete mMainWindow;
	delete mEngineConfig;
	delete mRenderer;
	delete mActiveScene;
}

void RZE_Engine::PostExit()
{
	bShouldExit = true;
}

ResourceHandler& RZE_Engine::GetResourceHandler()
{
	return mResourceHandler;
}

InputHandler& RZE_Engine::GetInputHandler()
{
	return mInputHandler;
}

Diotima::Renderer& RZE_Engine::GetRenderer()
{
	AssertNotNull(mRenderer);
	return *mRenderer;
}

GameScene& RZE_Engine::GetActiveScene()
{
	AssertNotNull(mActiveScene);
	return *mActiveScene;
}

void RZE_Engine::Log(const std::string& text, const Vector3D& color)
{
}

