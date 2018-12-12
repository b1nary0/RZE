#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>
#include <Utils/Platform/FilePath.h>

// TEST
#include <Diotima/Graphics/Texture2D.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <DebugUtils/DebugServices.h>

// GAME
#include <Game/Systems/FirstPersonCameraSystem.h>
#include <Game/Systems/InteractiveSpawningSystem.h>

#include <random>

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

	RZE().GetActiveScene().Load(FilePath("Engine/Assets/Scenes/TestGame.scene"));

	//RZE().GetActiveScene().GetEntityHandler().AddSystem<InteractiveSpawningSystem>();
	RZE().GetActiveScene().GetEntityHandler().AddSystem<FirstPersonCameraSystem>();
}

void GameApp::Start()
{
	RZE_Application::Start();

	srand(time(NULL));
	for (int i = 0; i < 3000; ++i)
	{
		Apollo::EntityID entity = RZE_Application::RZE().GetActiveScene().CreateEntity("DefaultEntityName");
		RZE_Application::RZE().GetActiveScene().GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Engine/Assets/3D/FW190/FW190.obj"));

		TransformComponent* const transf = RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponent<TransformComponent>(entity);
		transf->Position = Vector3D((rand() + 1) % 25, (rand() + 1) % 25, (rand() + 1) % 25);
	}
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
	});
	inputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
}
