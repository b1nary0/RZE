#include "GameApp.h"

#include <RZE.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>
#include <Utils/Platform/FilePath.h>

// TEST
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <ECS/Systems/FreeCameraSystem.h>

#include <DebugUtils/DebugServices.h>

// GAME
#include <Game/Systems/FirstPersonCameraSystem.h>
#include <Game/Systems/InteractiveSpawningSystem.h>
#include <Game/Systems/ProjectileSystem.h>
#include <Game/Systems/TestbedSystem.h>

#include <Game/Components/VelocityComponent.h>

GameApp::GameApp()
	: RZE_Application()
{
}

GameApp::~GameApp()
{
}

void GameApp::Initialize()
{
	RZE_Application::Initialize();

	APOLLO_REGISTER_COMPONENT(VelocityComponent);

	// #TODO(Josh::Putting this comment here because too lazy to do it elsewhere. Hopefully I find it later:
	//             -- Need to add at least commandline arg like -startscene or something)
	RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));

	Apollo::EntityHandler& entityHandler = RZE().GetActiveScene().GetEntityHandler();
	//entityHandler.AddSystem<InteractiveSpawningSystem>();
	//entityHandler.AddSystem<TestbedSystem>();
	entityHandler.AddSystem<FirstPersonCameraSystem>();
	entityHandler.AddSystem<ProjectileSystem>();
}

void GameApp::Start()
{
	RZE_Application::Start();
}

void GameApp::Update()
{
	RZE_Application::Update();
}

void GameApp::ShutDown()
{
	RZE_Application::ShutDown();
}

void GameApp::RegisterInputEvents(InputHandler& inputHandler)
{
	Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
	{
		if (key.GetKeyCode() == Win32KeyCode::Escape)
		{
			RZE().PostExit();
		}
		else if (key.GetKeyCode() == Win32KeyCode::F1)
		{
			RZE().SetWindowSize(Vector2D(1280, 720));
		}
		else if (key.GetKeyCode() == Win32KeyCode::F2)
		{
			RZE().SetWindowSize(Vector2D(1600, 900));
		}
		else if (key.GetKeyCode() == Win32KeyCode::F3)
		{
			RZE().SetWindowSize(Vector2D(1920, 1080));
		}

		if (key.GetKeyCode() == Win32KeyCode::Key_K)
		{
			for (U32 entityCount = 0; entityCount < 3000; ++entityCount)
			{
				Apollo::EntityID entity = RZE().GetActiveScene().CreateEntity("Entity");
				RZE().GetActiveScene().GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Assets/3D/FW190/FW190.obj"));
			}
		}
	});
	inputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F1, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F2, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F3, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::Key_K, EButtonState::ButtonState_Pressed, keyFunc);
}
