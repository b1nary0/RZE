#include "StdAfx.h"

#include "Application.h"

#include "Debug/Debug.h"

#include "EngineCore/Engine.h"

RZE_Game::RZE_Game()
	: bIsRunning(false)
{
	Init();
}

RZE_Game::~RZE_Game()
{

}

void RZE_Game::Start()
{
	bIsRunning = true;
}

bool RZE_Game::IsRunning() const
{
	return bIsRunning;
}

void RZE_Game::Update()
{
}

void RZE_Game::Init()
{
	LOG_CONSOLE("RZE_Application::Init() called. \n");
}

void RZE_Game::RegisterEvents(EventHandler& eventHandler)
{
}
