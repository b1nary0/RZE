#include "StdAfx.h"

#include "Engine.h"

UInt8 RZE_EngineCore::sInstanceCount = 0;

RZE_EngineCore::RZE_EngineCore()
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

RZE_EngineCore::~RZE_EngineCore()
{
}

std::weak_ptr<Win32Window> RZE_EngineCore::MakeWindow(const std::string& title, const int width, const int height)
{
	return mWindowManager.MakeWindow(title, width, height);
}

void RZE_EngineCore::Run(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback)
{
	PostInit(createApplicationCallback);

	while (!bShouldExit)
	{
		Update();

		mApplication->Update();
	}
}

void RZE_EngineCore::Init()
{
	printf("RZE_EngineCore::Init() called. \n");
	
	RegisterEvents();

	mMainWindow = MakeWindow("RZE_Application", 1280, 720);
}

void RZE_EngineCore::PostInit(Functor<std::unique_ptr<RZE_Application>> createApplicationCallback)
{
	printf("RZE_EngineCore::PostInit() called. \n");

	mApplication = createApplicationCallback();
	mApplication->RegisterEvents(mEventHandler);
}

void RZE_EngineCore::CompileEvents()
{
	mWindowManager.CompileEvents(mEventHandler);
}

void RZE_EngineCore::RegisterEvents()
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

void RZE_EngineCore::Update()
{
	mEventHandler.ProcessEvents();

	CompileEvents();
}

void RZE_EngineCore::ShutDown()
{
	printf("Shutting engine down...\n");
}

void RZE_EngineCore::RegisterForEvent(const UInt16 eventType, Functor<void, const Event&> callback)
{
	mEventHandler.RegisterForEvent(eventType, callback);
}
