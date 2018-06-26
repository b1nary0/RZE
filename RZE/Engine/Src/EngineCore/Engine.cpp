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

#include <Apollo/EntityComponentSystem.h>
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
		double currentTime = programTimer.GetElapsed<double>();
		while (!bShouldExit)
		{
			double newTime = programTimer.GetElapsed<double>();
			double frameTime = newTime - currentTime;
			currentTime = newTime;

			mDeltaTime = frameTime;
			PreUpdate();
			// #TODO(Josh) Need to work this out but for the moment we need to pre update and then start the imgui new frame for things like editor stealing imgui input etc
			ImGui::NewFrame();

			DebugServices::AddData(StringUtils::FormatString("Frame Time: %f ms", static_cast<float>(mDeltaTime) * 1000.0f), Vector3D(1.0f, 1.0f, 0.0f));
			{
				Update();
				
				DebugServices::Display(GetWindowSize());

				mRenderer->Update();

				ImGui::Render();
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

		LoadEngineConfig();

		CreateAndInitializeWindow();

		mInputHandler.Initialize();

		RegisterWindowEvents();
		RegisterEngineComponentTypes();
		
		mRenderer = new Diotima::Renderer();
		mRenderer->Initialize();
		mRenderer->EnableVsync(mEngineConfig->GetEngineSettings().IsVSyncEnabled());

		mResourceHandler.Init();

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

	// #TODO(Josh) Test
	mActiveScene->Load(FilePath("Engine/Assets/Scenes/Test.scene"));
	mActiveScene->Start();
}

void RZE_Engine::PreUpdate()
{
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

void RZE_Engine::InitializeApplication(Functor<RZE_Application* const> createGameCallback)
{
	mApplication = createGameCallback();
	AssertNotNull(mApplication);

	mApplication->Initialize();
	mApplication->RegisterInputEvents(mInputHandler);
	mApplication->SetWindow(mMainWindow);
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
			GetRenderer()->ResizeCanvas(newSize);
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
{
	mActiveScene->Update();
	mApplication->Update();
}

void RZE_Engine::BeginShutDown()
{
	LOG_CONSOLE("Shutting engine down...");
	
	mActiveScene->Finish();
	mApplication->ShutDown();
	mResourceHandler.ShutDown();

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

GameScene& RZE_Engine::GetActiveScene()
{
	AssertNotNull(mActiveScene);
	return *mActiveScene;
}

void RZE_Engine::Log(const std::string& text, const Vector3D& color)
{
}

