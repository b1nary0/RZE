#include <StdAfx.h>
#include <EngineCore/Engine.h>

#include <imGUI/imgui.h>

#include <DebugUtils/DebugServices.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <ECS/Systems/RenderSystem.h>

#include <Editor/Editor.h>

#include <Windowing/Win32Window.h>
#include <Windowing/WinKeyCodes.h>

#include <Utils/DebugUtils/Debug.h>

#include <Apollo/EntityComponentSystem.h>
#include <Diotima/Renderer.h>

RZE_Engine* RZE_Engine::sInstance = nullptr;

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

		HiResTimer programTimer;
		programTimer.Start();
		double currentTime = programTimer.GetElapsed<double>();
		while (!bShouldExit)
		{
			double newTime = programTimer.GetElapsed<double>();
			double frameTime = newTime - currentTime;
			currentTime = newTime;

			DebugServices::AddData(StringUtils::FormatString("Frame Time: %f ms", static_cast<float>(mDeltaTime) * 1000.0f), Vector3D(1.0f, 1.0f, 0.0f));
			{
				mDeltaTime = frameTime;

				PreUpdate();
				Update();

#if EDITOR
				mEditor->Display();
#endif

				mRenderer->Update();
				DebugServices::Display(GetWindowSize());
			}
			ImGui::Render();
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

void RZE_Engine::Init()
{
	if (!IsInitialized())
	{
		LOG_CONSOLE("RZE_EngineCore::Init() called.");

		LoadEngineConfig();

		CreateAndInitializeWindow();

		DebugServices::Initialize();
		DebugServices::HandleScreenResize(GetWindowSize());

		mInputHandler.Initialize();

		RegisterWindowEvents();
		RegisterInputEvents();
		RegisterEngineComponentTypes();

		mRenderer = new Diotima::Renderer();
		mRenderer->Initialize();
		mRenderer->EnableVsync(mEngineConfig->GetEngineSettings().IsVSyncEnabled());

		mResourceHandler.Init();

#if EDITOR
		mEditor = new RZE_Editor();
		mEditor->Initialize();
#endif

		mActiveScene = new GameScene();
		mActiveScene->Initialize();

		bIsInitialized = true;
	}
}

void RZE_Engine::PostInit(Functor<RZE_Game* const>& createApplicationCallback)
{
	LOG_CONSOLE("RZE_EngineCore::PostInit() called.");

	InitGame(createApplicationCallback);

	mActiveScene->Start();
}

void RZE_Engine::PreUpdate()
{
	CompileEvents();
	mEventHandler.ProcessEvents();
	mInputHandler.RaiseEvents();

	{
		ImGuiIO& io = ImGui::GetIO();

		io.DeltaTime = mDeltaTime;

		io.MousePos = ImVec2(GetInputHandler().GetMouseState().CurPosition.X(), GetInputHandler().GetMouseState().CurPosition.Y());

		for (int i = 0; i < 3; ++i)
		{
			io.MouseDown[i] = GetInputHandler().GetMouseState().CurMouseBtnStates[i];
		}

		ImGui::NewFrame();
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

	mMainWindow->ResetCursorToCenter();
}

void RZE_Engine::InitGame(Functor<RZE_Game* const> createGameCallback)
{
	mApplication = createGameCallback();
	AssertNotNull(mApplication);

	mApplication->SetWindow(mMainWindow);
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
			GetRenderer()->ResizeCanvas(newSize);
			DebugServices::HandleScreenResize(newSize);
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
		else
		{
			RZE_Engine::Get()->Log("F1 pressed", Vector3D(1.0f, 1.0f, 0.0f));
		}
	});

	mInputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, inputFunc);
	mInputHandler.BindAction(Win32KeyCode::F1, EButtonState::ButtonState_Pressed, inputFunc);
}

void RZE_Engine::RegisterEngineComponentTypes()
{
	APOLLO_REGISTER_COMPONENT(CameraComponent);
	APOLLO_REGISTER_COMPONENT(LightSourceComponent);
	APOLLO_REGISTER_COMPONENT(MaterialComponent);
	APOLLO_REGISTER_COMPONENT(MeshComponent);
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
	mApplication->Update();
	mActiveScene->Update();
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
	AssertNotNull(mApplication);
	AssertNotNull(mRenderer);
	AssertNotNull(mActiveScene);

	delete mMainWindow;
	delete mEngineConfig;
	delete mApplication;
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
#if EDITOR
	mEditor->GetLogWidget().AddEntry(text, color);
#endif
}

