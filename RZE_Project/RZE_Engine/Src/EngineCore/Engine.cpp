#include "StdAfx.h"

#include "Engine.h"

#include "Debug/Debug.h"

#include "Windowing/Win32Window.h"
#include "Windowing/WinKeyCodes.h"

UInt8 RZE_Engine::sInstanceCount = 0;

RZE_Engine::RZE_Engine()
	: bShouldExit(false)
{
	++sInstanceCount;
	assert(sInstanceCount == 1 && "Trying to create second engine class, invalid operation. Exiting.");
	if (sInstanceCount != 1)
	{
		exit(0);
	}

	Init();
}

RZE_Engine::~RZE_Engine()
{
}

void RZE_Engine::Run(Functor<std::unique_ptr<RZE_Game>> createApplicationCallback)
{
	PostInit(createApplicationCallback);

	while (!bShouldExit)
	{
		Update();

		mApplication->Update();
	}
}

void RZE_Engine::Init()
{
	LOG_CONSOLE("RZE_EngineCore::Init() called. \n");

	mRenderer.MakeWindow("RZE_Engine", 1280, 720);

	RegisterWindowEvents();
	RegisterInputEvents();

	mInputHandler.RegisterEvents(mEventHandler);
}

void RZE_Engine::PostInit(Functor<std::unique_ptr<RZE_Game>> createApplicationCallback)
{
	LOG_CONSOLE("RZE_EngineCore::PostInit() called. \n");

	mApplication = createApplicationCallback();
	mApplication->RegisterEvents(mEventHandler);
}

void RZE_Engine::CompileEvents()
{
	std::shared_ptr<Win32Window> mainWindow = mRenderer.GetMainWindow().lock();
	if (mainWindow)
	{
		mainWindow->CompileMessages(mEventHandler);
	}
	else
	{
		assert(false && "Had trouble locking window from renderer. This is a problem.");
	}
}

void RZE_Engine::RegisterWindowEvents()
{
	Functor<void, const Event&> windowCallback([this](const Event& event)
	{
		assert(event.mInfo.mEventType == EEventType::Window);
		if (event.mWindowEvent.mEventInfo.mEventSubType == EWindowEventType::Window_Destroy)
		{
			this->bShouldExit = true;
		}
	});

	RegisterForEvent(EEventType::Window, windowCallback);
}

void RZE_Engine::RegisterInputEvents()
{
	Functor<void, UInt8> keyPressCallback([this](const UInt8 key)
	{
		if (key == Win32KeyCode::Escape)
		{
			this->bShouldExit = true;
		}
	});

	mInputHandler.RegisterForEvent(EKeyEventType::Key_Pressed, keyPressCallback);
}

void RZE_Engine::Update()
{
	mEventHandler.ProcessEvents();
	mInputHandler.ProcessEvents();

	mRenderer.Render();

	CompileEvents();
}

void RZE_Engine::ShutDown()
{
	LOG_CONSOLE("Shutting engine down...\n");
}

void RZE_Engine::RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback)
{
	mEventHandler.RegisterForEvent(eventType, callback);
}

std::weak_ptr<Win32Window> RZE_Engine::GetMainWindow() const
{
	return mRenderer.GetMainWindow();
}
