#include <StdAfx.h>

#include <EngineCore/Engine.h>
#include <EngineCore/Platform/Timers/HiResTimer.h>

#include <DebugUtils/Debug.h>

#include <Game/GameWorld.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

#include <imGUI/imgui.h>

RZE_Engine* RZE_Engine::sInstance = nullptr;

RZE_Engine::RZE_Engine()
{
	mMainWindow = nullptr;
	mRenderer = nullptr;
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

		static int frameCount = 0;

		HiResTimer updateTimer;
		HiResTimer renderTimer;
		while (!bShouldExit)
		{
			//ImGui::NewFrame();

			updateTimer.Start();
			Update();
			updateTimer.Stop();

			renderTimer.Start();
			mRenderer->Render();
			renderTimer.Stop();

			++frameCount;

			float updateTime = updateTimer.GetElapsed<float>() * 1000;
			if (frameCount > 25)
			{
				updateTime = updateTimer.GetElapsed<float>() * 1000;
				frameCount = 0;
			}

			//ImGui::Render();
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
	LOG_CONSOLE("RZE_EngineCore::Init() called.");

	LoadEngineConfig();

	CreateAndInitializeWindow();

	{
		OpenGLRHI::OpenGLCreationParams creationParams;
		creationParams.WindowWidth = static_cast<int>(mMainWindow->GetDimensions().X());
		creationParams.WindowHeight = static_cast<int>(mMainWindow->GetDimensions().Y());

		OpenGLRHI::Get().Init(creationParams);
	}

	// #TODO this should be handled better. No need to create directly here. Take a look.
	mRenderer = new RZE_Renderer();

	LoadFonts();

	mResourceHandler.Init();
	mInputHandler.Initialize();

	RegisterWindowEvents();
	RegisterInputEvents();

	bIsInitialized = true;
}

void RZE_Engine::InitWorld()
{
	LOG_CONSOLE_ANNOUNCE("Initializing Game World...");

	// @note naive at first
	mWorld = new GameWorld(mRenderer);
	AssertNotNull(mWorld);

	mWorld->Init();
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

	mMainWindow->ResetCursorToCenter();
}

void RZE_Engine::LoadFonts()
{
	mFontHandler.LoadFont("Arial", "./../Engine/Assets/Fonts/Arial.ttf");
}

void RZE_Engine::InitGame(Functor<RZE_Game* const> createGameCallback)
{
	mApplication = createGameCallback();
	AssertNotNull(mApplication);

	mApplication->SetWindow(mMainWindow);
	mApplication->Start();

	mApplication->RegisterEvents(mEventHandler);
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
			U16 width = event.mWindowEvent.mSizeX;
			U16 height = event.mWindowEvent.mSizeY;
			mRenderer->ResizeCanvas(Vector2D(width, height));
		}
	});
	mEventHandler.RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::RegisterInputEvents()
{
	Functor<void, U8> keyPressCallback([this](const U8 key)
	{
		if (key == Win32KeyCode::Escape)
		{
			PostExit();
		}
	});
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

	InternalShutDown();
}

void RZE_Engine::InternalShutDown()
{
	AssertNotNull(mRenderer);
	AssertNotNull(mMainWindow);
	AssertNotNull(mEngineConfig);
	AssertNotNull(mApplication);
	AssertNotNull(mWorld);

	delete mRenderer;
	delete mMainWindow;
	delete mEngineConfig;
	delete mApplication;
	delete mWorld;
}

void RZE_Engine::PostExit()
{
	bShouldExit = true;
}

SceneCamera& RZE_Engine::GetSceneCamera()
{
	AssertNotNull(mRenderer);
	return mRenderer->GetSceneCamera();
}

WindowSettings& RZE_Engine::GetWindowSettings()
{
	return mEngineConfig->GetWindowSettings();
}

GameWorld& RZE_Engine::GetWorld() const
{
	AssertNotNull(mWorld);
	return *mWorld;
}

ResourceHandler& RZE_Engine::GetResourceHandler()
{
	return mResourceHandler;
}

FontHandler& RZE_Engine::GetFontHandler()
{
	return mFontHandler;
}

void RZE_Engine::SetCursorEnabled(bool enabled)
{
	if (mMainWindow)
	{
		mMainWindow->SetCursorEnabled(enabled);
	}
}

