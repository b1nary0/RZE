#include "StdAfx.h"

#include "EngineApp.h"

#include "DebugUtils/Debug.h"

#include "EngineCore/Engine.h"

#include "Windowing/Win32Window.h"

RZE_Game::RZE_Game()
	: bIsRunning(false)
	, mWindow(nullptr)
{
	Init();
}

RZE_Game::~RZE_Game()
{

}

void RZE_Game::Start()
{
	ShowWindow();

	bIsRunning = true;
}

bool RZE_Game::IsRunning() const
{
	return bIsRunning;
}

void RZE_Game::Update()
{
}

void RZE_Game::ShutDown()
{
}

void RZE_Game::Init()
{
	LOG_CONSOLE("RZE_Application::Init() called.");
}

void RZE_Game::ShowWindow()
{
	AssertNotNull(mWindow);
	mWindow->Show();
}

void RZE_Game::SetWindow(Win32Window* const window)
{
	AssertNotNull(window);
	mWindow = window;
}

Win32Window* const RZE_Game::GetWindow() const
{
	return mWindow;
}

void RZE_Game::CompileEvents(EventHandler& eventHandler)
{
	mWindow->CompileMessages(eventHandler);
}

void RZE_Game::RegisterEvents(EventHandler& eventHandler)
{
}
