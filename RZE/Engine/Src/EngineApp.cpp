#include <StdAfx.h>
#include <EngineApp.h>

#include <Diotima/Driver/OpenGL/GLRenderTarget.h>

#include <EngineCore/Engine.h>

#include <Windowing/Win32Window.h>

#include <Utils/DebugUtils/Debug.h>

RZE_Engine RZE_Application::mEngine;

RZE_Application::RZE_Application()
	: mWindow(nullptr)
{
	SetRunning(false);
}

RZE_Application::~RZE_Application()
{

}

void RZE_Application::Run()
{
	SetRunning(true);

	Functor<RZE_Application* const> engineHook([this]()
	{
		return this;
	});

	RZE().Run(engineHook);
}

void RZE_Application::Start()
{

}

void RZE_Application::Update()
{
}

void RZE_Application::ShutDown()
{
	AssertNotNull(mRenderTarget);
	delete mRenderTarget;
}

bool RZE_Application::ProcessInput(const InputHandler& handler)
{
	return true;
}

void RZE_Application::RegisterInputEvents(InputHandler& inputHandler)
{

}

Diotima::RenderTarget& RZE_Application::GetRenderTarget() const
{
	AssertNotNull(mRenderTarget);
	return *mRenderTarget;
}

void RZE_Application::Initialize()
{
	LOG_CONSOLE("RZE_Application::Init() called.");

	// #TODO(Josh) With proper abstraction + platform tools, this should go away/become more versatile but for now just GL
	mRenderTarget = new Diotima::GLRenderTargetTexture();
}

Win32Window& RZE_Application::InternalGetWindow()
{
	AssertNotNull(mWindow);
	return *mWindow;
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
