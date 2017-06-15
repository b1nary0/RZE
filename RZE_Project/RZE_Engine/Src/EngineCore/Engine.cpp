#include <StdAfx.h>

#include <EngineCore/Engine.h>

#include <DebugUtils/Debug.h>

#include <Game/GameWorld.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

RZE_Engine* RZE_Engine::sInstance = nullptr;

RZE_Engine::RZE_Engine()
	: bShouldExit(false)
	, mMainWindow(nullptr)
{
	Init();
}

RZE_Engine::~RZE_Engine()
{
	AssertNotNull(mMainWindow);
	delete mMainWindow;
}

void RZE_Engine::Run(Functor<RZE_Game* const>& createGameCallback)
{
	AssertNotNull(createGameCallback);
	PostInit(createGameCallback);

	while (!bShouldExit)
	{
		Update();
	}

	BeginShutDown();
}

void RZE_Engine::Init()
{
	LOG_CONSOLE("RZE_EngineCore::Init() called.");
	
	CreateAndInitializeWindow();
	OpenGLRHI::Get().Init();

	mRenderer = new RZE_Renderer();

	RegisterWindowEvents();
	RegisterInputEvents();

	mInputHandler.RegisterEvents(mEventHandler);
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
	Win32Window::WindowCreationParams params;
	params.windowTitle = "RZE_Engine";
	params.width = 1280;
	params.height = 720;

	mMainWindow = new Win32Window(params);
	AssertNotNull(mMainWindow);
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
	mApplication->CompileEvents(mEventHandler);
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
	});
	RegisterForEvent(EEventType::Window, windowCallback);
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
	RegisterForInputEvent(EKeyEventType::Key_Pressed, keyPressCallback);
}

void RZE_Engine::Update()
{
	mEventHandler.ProcessEvents();
	mInputHandler.ProcessEvents();

	mApplication->Update();
	mWorld->Update();

	mRenderer->Render();
	// @todo maybe this can be done better
	mMainWindow->BufferSwap();

	CompileEvents();
}

void RZE_Engine::BeginShutDown()
{
	LOG_CONSOLE("Shutting engine down...");
	mWorld->ShutDown();
	mApplication->ShutDown();
}

void RZE_Engine::PostExit()
{
	bShouldExit = true;
}

void RZE_Engine::RegisterForEvent(const U16 eventType, Functor<void, const Event&> callback)
{
	mEventHandler.RegisterForEvent(eventType, callback);
}

void RZE_Engine::RegisterForInputEvent(const U16 eventType, Functor<void, U8> callback)
{
	mInputHandler.RegisterForEvent(eventType, callback);
}

GameWorld* const RZE_Engine::GetWorld() const
{
	AssertNotNull(mWorld);
	return mWorld;
}
