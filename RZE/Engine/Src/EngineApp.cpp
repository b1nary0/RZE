#include <EngineApp.h>

#include <EngineCore/Engine.h>

#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/GifChatComponent.h>
#include <Game/World/GameObjectComponents/LightComponent.h>
#include <Game/World/GameObjectComponents/ReflectionProbeComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Windowing/Window.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/CmdLine.h>

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

void RZE_Application::PreUIUpdate()
{
}

void RZE_Application::UpdateUI()
{
}

void RZE_Application::Update()
{
}

void RZE_Application::PreRender()
{
}

void RZE_Application::ShutDown()
{

}

Filepath RZE_Application::GetSplashScreenImage() const
{
	return Filepath();
}

std::string RZE_Application::GetWindowTitle() const
{
	return std::string();
}

bool RZE_Application::IsReadyToShow() const
{
	return true;
}

InputGate RZE_Application::ProcessInput(const InputHandler& handler)
{
	return InputGate{};
}

void RZE_Application::RegisterInputEvents(InputHandler& inputHandler)
{
}

void RZE_Application::Initialize()
{
	RZE_LOG("RZE_Application::Init() called.");

	RegisterEngineComponents();
}

void RZE_Application::RegisterEngineComponents()
{
	// #TODO This basically just runs a bunch of poor mans reflection stuff like
	// creating entries in the component name lookup and factory functions. Necessary to function atm.
	// Eventually hope to implement actual reflection and handle this.
	REGISTER_GAMEOBJECTCOMPONENT(CameraComponent);
	REGISTER_GAMEOBJECTCOMPONENT(RenderComponent);
	REGISTER_GAMEOBJECTCOMPONENT(TransformComponent);
	REGISTER_GAMEOBJECTCOMPONENT(LightComponent);
	REGISTER_GAMEOBJECTCOMPONENT(ReflectionProbeComponent);
	REGISTER_GAMEOBJECTCOMPONENT(GifChatComponent);

	// Lets async scene loading preload these components' resources on worker threads.
	GameObjectComponentRegistry::SetResourceDependencyGatherer(RenderComponent::StaticTypeID(), &RenderComponent::GatherResourceDependencies);
}

Window& RZE_Application::InternalGetWindow()
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

void RZE_Application::SetWindow(Window* const window)
{
	AssertNotNull(window);
	mWindow = window;
}

Window* const RZE_Application::GetWindow() const
{
	return mWindow;
}

Rendering::RenderTargetTexture* RZE_Application::GetRTT()
{
	AssertNotNull(m_renderTarget);
	return m_renderTarget.get();
}

void RZE_Application::ParseArguments(char** arguments, int count)
{
	CmdLine::Arguments::Initialize(arguments, count);

	// The Editor and the Game are windowed apps with no console of their own
	std::string_view consoleValue;
	if (CmdLine::Arguments::Get("-console", consoleValue))
	{
		Debug::OpenConsole();
	}
}
