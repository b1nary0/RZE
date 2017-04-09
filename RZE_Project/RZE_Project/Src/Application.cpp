#include "StdAfx.h"

#include "Application.h"

#include "EngineCore/Engine.h"

RZE_Application::RZE_Application()
	: bIsRunning(false)
{
	mEngineCore = std::make_unique<RZE_EngineCore>();

	Init();
}

RZE_Application::~RZE_Application()
{

}

void RZE_Application::Start()
{
	bIsRunning = true;
}

bool RZE_Application::IsRunning() const
{
	return bIsRunning;
}

void RZE_Application::Update()
{
	mEngineCore->Update();
}

void RZE_Application::Init()
{
	printf("RZE_Application::Init() called. \n");

	mWindow = mEngineCore->MakeWindow("RZE_Application", 1280, 720);

	RegisterEvents();
}

void RZE_Application::RegisterEvents()
{
	std::unique_ptr<RZE_EngineCore>& engineCore = mEngineCore;
	auto shutdownFunc = [&engineCore, this](const Event& event)
	{
		if (event.mInfo.mEventType == 0)
		{
			if (event.mInfo.mEventSubType == 1)
			{
				engineCore->ShutDown();
				this->bIsRunning = false;
			}
		}
	};

	mEngineCore->RegisterForEvent(0, Functor<void, const Event&>(shutdownFunc));
}
