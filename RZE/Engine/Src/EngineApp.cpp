#include <StdAfx.h>
#include <EngineApp.h>

#include <EngineCore/Engine.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LifetimeComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MaterialComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Windowing/Win32Window.h>

#include <Rendering/Driver/DX11/DX11GFXDevice.h>
#include <Rendering/Graphics/RenderTarget.h>

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

}

bool RZE_Application::ProcessInput(const InputHandler& handler)
{
	return true;
}

void RZE_Application::RegisterInputEvents(InputHandler& inputHandler)
{
}

void RZE_Application::Initialize()
{
	LOG_CONSOLE("RZE_Application::Init() called.");

	//APOLLO_REGISTER_COMPONENT(CameraComponent);
	//APOLLO_REGISTER_COMPONENT(LightSourceComponent);
	//APOLLO_REGISTER_COMPONENT(LifetimeComponent);
	//APOLLO_REGISTER_COMPONENT(MaterialComponent);
	//APOLLO_REGISTER_COMPONENT(MeshComponent);
	//APOLLO_REGISTER_COMPONENT(NameComponent);
	//APOLLO_REGISTER_COMPONENT(TransformComponent);
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

Rendering::RenderTargetTexture* RZE_Application::GetRTT()
{
	AssertNotNull(mRenderTarget);
	return mRenderTarget.get();
}

void RZE_Application::ParseArguments(const char* arguments, int count)
{
}
