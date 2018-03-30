#include <StdAfx.h>

#include <EngineApp.h>

#include <EngineCore/Engine.h>
#include <Windowing/Win32Window.h>

#include <Utils/DebugUtils/Debug.h>

RZE_Application::RZE_Application()
	: mWindow(nullptr)
{
	SetRunning(false);

	Init();
}

RZE_Application::~RZE_Application()
{

}

void RZE_Application::Start()
{
	ShowWindow();

	SetRunning(true);
}

void RZE_Application::Update()
{
}

void RZE_Application::ShutDown()
{
}

void RZE_Application::Init()
{
	LOG_CONSOLE("RZE_Application::Init() called.");
}

void RZE_Application::SetRunning(bool bRunning)
{
	bIsRunning = bRunning;
}

void RZE_Application::ShowWindow()
{
	AssertNotNull(mWindow);
	mWindow->Show();
}

void RZE_Application::SetWindow(Win32Window* const window)
{
	AssertNotNull(window);
	mWindow = window;
}

Win32Window* const RZE_Application::GetWindow() const
{
	return mWindow;
}
