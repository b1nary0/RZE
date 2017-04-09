#include "StdAfx.h"

#include "Application.h"

#include "EngineCore/Engine.h"

RZE_Application::RZE_Application()
	: bIsRunning(false)
{
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
}

void RZE_Application::Init()
{
	printf("RZE_Application::Init() called. \n");
}

void RZE_Application::RegisterEvents(EventHandler& eventHandler)
{
}
