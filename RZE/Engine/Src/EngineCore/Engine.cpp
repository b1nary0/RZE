#include <StdAfx.h>

#include <EngineCore/Engine.h>
#include <EngineCore/Platform/Timers/HiResTimer.h>

#include <DebugUtils/Debug.h>
#include <DebugUtils/DebugServices.h>

#include <Diotima/RenderSystem.h>

#include <Game/GameWorld.h>

#include <RenderCore/Renderer.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

#include <imGUI/imgui.h>

void TempInitImGui()
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = RZE_Engine::Get()->GetWindowSettings().GetDimensions().X();
	io.DisplaySize.y = RZE_Engine::Get()->GetWindowSettings().GetDimensions().Y();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

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
	bDisplayDebugServices = false;

	mFrameCount = 0;

	for (int i = 0; i < MAX_FPS_SAMPLES; ++i)
	{
		mFPSSamples[i] = 0;
	}
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

		HiResTimer updateTimer;
		HiResTimer renderTimer;
		HiResTimer frameTimer;

		float prevFrameTime = 1.0f;
		float curFrameTime = 1.0f;
		float accumulator = 0.0f;
		const float kMaxDeltaTime = 1/240.0f;

		float updateTime = 0.0f;
		float renderTime = 0.0f;
		float deltaTimeStat = 0.0f;
		float fps = 0.0f;
		bool bUpdateFrameStats = false;

		frameTimer.Start();
		while (!bShouldExit)
		{
			prevFrameTime = curFrameTime;
			curFrameTime = frameTimer.GetElapsed<float>();
			mDeltaTime = curFrameTime - prevFrameTime;
			accumulator += mDeltaTime;

			++mFrameCount;

			ImGui::NewFrame();

			{
				while (accumulator >= kMaxDeltaTime)
				{
					updateTimer.Start();
					Update();
					updateTimer.Stop();

					accumulator -= kMaxDeltaTime;
				}

				renderTimer.Start();
				mRenderer->Render();
				renderTimer.Stop();
			}

			bUpdateFrameStats = mFrameCount % 5 == 0;
			if (bUpdateFrameStats)
			{
				fps = CalculateAvgFPS(mDeltaTime);
				updateTime = updateTimer.GetElapsed<float>() * 1000.0f;
				renderTime = renderTimer.GetElapsed<float>() * 1000.0f;
				deltaTimeStat = mDeltaTime * 1000.0f;
			}

			DebugServices::AddData(StringUtils::FormatString("FPS: %i", static_cast<int>(fps)), Vector3D(0.0f, 1.0f, 0.0f));
			DebugServices::AddData(StringUtils::FormatString("Frame Time: %f ms", deltaTimeStat), Vector3D(0.0f, 1.0f, 0.0f));
			DebugServices::AddData(StringUtils::FormatString("Update Time: %f ms", updateTime), Vector3D(0.0f, 1.0f, 0.0f));
			DebugServices::AddData(StringUtils::FormatString("Render Time: %f ms", renderTime), Vector3D(0.0f, 1.0f, 0.0f));

			if (ShouldDisplayDebugServices())
			{
				DebugServices::Display(GetMainWindowSize());
			}

			ImGui::Render();
			
			mMainWindow->BufferSwap(); // #TODO(Josh) Maybe this can be done better
		}
		frameTimer.Stop();

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

		{
			OpenGLRHI::OpenGLCreationParams creationParams;
			creationParams.WindowWidth = static_cast<int>(mMainWindow->GetDimensions().X());
			creationParams.WindowHeight = static_cast<int>(mMainWindow->GetDimensions().Y());

			OpenGLRHI::Get().Init(creationParams);
		}

		// #TODO this should be handled better. No need to create directly here. Take a look.
		mRenderer = new RZE_Renderer();

		TempInitImGui();

		LoadFonts();

		mResourceHandler.Init();
		mInputHandler.Initialize();

		RegisterWindowEvents();
		RegisterInputEvents();

		bIsInitialized = true;
	}
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

	mMainWindow->RegisterEvents(mEventHandler);

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
	mApplication->RegisterInputEvents(mInputHandler);
}

float RZE_Engine::CalculateAvgFPS(float prevElapsed)
{
	mFPSSamples[mFrameCount % MAX_FPS_SAMPLES] = 1.0f / prevElapsed;
	float fps = 0.0f;
	for (int frameSampleIdx = 0; frameSampleIdx < MAX_FPS_SAMPLES; frameSampleIdx++)
	{
		fps += mFPSSamples[frameSampleIdx];
	}

	fps /= MAX_FPS_SAMPLES;

	return fps;
}

void RZE_Engine::CompileEvents()
{
	mMainWindow->CompileInputMessages(mInputHandler);
	mMainWindow->CompileWindowMessages(mEventHandler);
}

void RZE_Engine::RegisterSubSystems()
{
	mRenderSystemId = mSubSystemHandler.AddSubSystem<Diotima::RenderSystem>();

	Diotima::RenderSystem* renderSystem = mSubSystemHandler.GetSubSystemByIndex<Diotima::RenderSystem>(mRenderSystemId);
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
		else if (key.GetKeyCode() == Win32KeyCode::F1)
		{
			SetDisplayDebugServices(!ShouldDisplayDebugServices());
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

const Vector2D& RZE_Engine::GetMainWindowSize() const
{
	return mMainWindow->GetDimensions();
}

void RZE_Engine::SetCursorEnabled(bool enabled)
{
	if (mMainWindow)
	{
		mMainWindow->SetCursorEnabled(enabled);
	}
}

