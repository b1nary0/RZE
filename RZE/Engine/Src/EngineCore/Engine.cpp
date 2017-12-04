#include <StdAfx.h>

#include <EngineCore/Engine.h>

#include <DebugUtils/DebugServices.h>

#include <ECS/Systems/RenderSystem.h>

#include <Game/GameWorld.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

#include <Utils//Platform/Timers/HiResTimer.h>
#include <Utils/DebugUtils/Debug.h>

// External Libs
#include <imGUI/imgui.h>

#include <Apollo/EntityComponentSystem.h>

#include <Diotima/RenderSystem.h>
#include <Diotima/Driver/OpenGL.h>

RZE_Engine* RZE_Engine::sInstance = nullptr;

RZE_Engine::RZE_Engine()
{
	mMainWindow = nullptr;
	mWorld = nullptr;
	mEngineConfig = nullptr;
	mApplication = nullptr;

	bShouldExit = false;
	bIsInitialized = false;
}

RZE_Engine::~RZE_Engine()
{
	AssertNotNull(mMainWindow);
	delete mMainWindow;
}

void RZE_Engine::Run(Functor<RZE_Game* const>& createGameCallback)
{
	Init();

	if (bIsInitialized)
	{
		AssertNotNull(createGameCallback);
		PostInit(createGameCallback);

		while (!bShouldExit)
		{
			{
				Update();
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

void RZE_Engine::Init()
{
	if (!IsInitialized())
	{
		LOG_CONSOLE("RZE_EngineCore::Init() called.");

		LoadEngineConfig();

		CreateAndInitializeWindow();

		RegisterSubSystems();

		mSubSystemHandler.InitializeSubSystems();
		mResourceHandler.Init();
		mInputHandler.Initialize();

		mComponentHandler.AddSystem<RenderSystem>();

		RegisterWindowEvents();
		RegisterInputEvents();

		bIsInitialized = true;
	}
}

void RZE_Engine::InitWorld()
{
	LOG_CONSOLE_ANNOUNCE("Initializing Game World...");

	// @note naive at first
	mWorld = new GameWorld();
	AssertNotNull(mWorld);

	mWorld->Initialize();
}

void RZE_Engine::PostInit(Functor<RZE_Game* const>& createApplicationCallback)
{
	LOG_CONSOLE("RZE_EngineCore::PostInit() called.");

	InitWorld();
	InitGame(createApplicationCallback);
}

void RZE_Engine::CreateAndInitializeWindow()
{
	// #TODO(Josh) Move this somewhere else and deal with it so the console ALWAYS shows up in a visible spot.
	SetWindowPos(GetConsoleWindow(), 0, 1024, 600, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

	WindowSettings& windowSettings = mEngineConfig->GetWindowSettings();

	Win32Window::WindowCreationParams params;
	params.windowTitle = windowSettings.GetTitle();
	params.width = static_cast<int>(windowSettings.GetDimensions().X());
	params.height = static_cast<int>(windowSettings.GetDimensions().Y());

	mMainWindow = new Win32Window(params);
	AssertNotNull(mMainWindow);

	mMainWindow->RegisterEvents(mEventHandler);

	mMainWindow->ResetCursorToCenter();
}

void RZE_Engine::InitGame(Functor<RZE_Game* const> createGameCallback)
{
	mApplication = createGameCallback();
	AssertNotNull(mApplication);

	mApplication->SetWindow(mMainWindow);
	mApplication->Start();

	//mApplication->RegisterInputEvents(mInputHandler);
}

void RZE_Engine::CompileEvents()
{
	mMainWindow->CompileInputMessages(mInputHandler);
	mMainWindow->CompileWindowMessages(mEventHandler);
}

void RZE_Engine::RegisterSubSystems()
{
	mSubSystemHandler.AddSubSystem<Diotima::RenderSystem>();
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
			U16 width = event.mWindowEvent.mSizeX;
			U16 height = event.mWindowEvent.mSizeY;
			DebugServices::HandleScreenResize(Vector2D(width, height));
		}
	});
	mEventHandler.RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::RegisterInputEvents()
{
	Functor<void, const InputKey&> inputFunc([this](const InputKey& key)
	{
		if (key.GetKeyCode() == Win32KeyCode::Escape)
		{
			PostExit();
		}
	});

	mInputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, inputFunc);
	mInputHandler.BindAction(Win32KeyCode::F1, EButtonState::ButtonState_Pressed, inputFunc);
}

void RZE_Engine::LoadEngineConfig()
{
	mEngineConfig = new EngineConfig();
	//#TODO the path here needs to be dealt with properly
	mEngineConfig->Load("./../Engine/RZE/Config/Engine.ini");

	if (mEngineConfig->Empty())
	{
		LOG_CONSOLE("Engine config could not load. Defaults were used.");
	}
}

void RZE_Engine::Update()
{
	CompileEvents();
	mEventHandler.ProcessEvents();

	mSubSystemHandler.UpdateSubSystems();

	mComponentHandler.Update();

	mApplication->Update();
	mWorld->Update();
}

void RZE_Engine::BeginShutDown()
{
	LOG_CONSOLE("Shutting engine down...");
	mApplication->ShutDown();
	mWorld->ShutDown();
	mResourceHandler.ShutDown();

	// #TODO(Josh) shut down renderer and window, etc
	mSubSystemHandler.ShutDownSubSystems();

	InternalShutDown();
}

void RZE_Engine::InternalShutDown()
{
	AssertNotNull(mMainWindow);
	AssertNotNull(mEngineConfig);
	AssertNotNull(mApplication);
	AssertNotNull(mWorld);

	delete mMainWindow;
	delete mEngineConfig;
	delete mApplication;
	delete mWorld;
}

void RZE_Engine::PostExit()
{
	bShouldExit = true;
}

WindowSettings& RZE_Engine::GetWindowSettings()
{
	return mEngineConfig->GetWindowSettings();
}

ResourceHandler& RZE_Engine::GetResourceHandler()
{
	return mResourceHandler;
}

Diotima::RenderSystem* RZE_Engine::GetRenderSystem()
{
	return mSubSystemHandler.GetSubSystemByIndex<Diotima::RenderSystem>(0);
}
