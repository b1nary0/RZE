#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <DebugUtils/DebugServices.h>

#include <Apollo/EntityComponentSystem.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderGroup.h>

#include <Windowing/WinKeyCodes.h>

#include <Utils/Math/Math.h>
#include <Utils/DebugUtils/Debug.h>


// TEST
#include <Ecs/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <Game/Systems/RotateSystem.h>

using namespace Diotima;

// Test stuff -- this stuff will likely be removed at some point
// or if it ends up being "permanent", implemented more sensibly.
static Vector3D cameraStartPos = Vector3D(0.0f, 1.0f, 15.0f);
static bool mIsCameraStartSequence = true;

GameApp::GameApp()
	: RZE_Game()
{
}

GameApp::~GameApp()
{
}

void GameApp::Start()
{
	RZE_Game::Start();

	// ALL TEST CODE

	Apollo::ComponentHandler& componentHandler = RZE_Engine::Get()->GetComponentHandler();

	componentHandler.AddSystem<RotateSystem>();

	// Leaving the for loop for testing purposes
	for (int i = 0; i < 1; ++i)
	{
		Apollo::EntityID entity = componentHandler.CreateEntity();

		componentHandler.AddComponent<MeshComponent>(entity, "./../Engine/Assets/3D/Quad.obj");
		componentHandler.AddComponent<TransformComponent>(entity, Vector3D(), Quaternion(), Vector3D(4.0f, 3.0f, 0.0f));
		componentHandler.AddComponent<MaterialComponent>(entity, "./../Engine/Assets/2D/Container.jpg");
	}

	Apollo::EntityID lightSource = componentHandler.CreateEntity();
	componentHandler.AddComponent<LightSourceComponent>(lightSource, Vector3D(1.0f, 1.0f, 1.0f), 1.0f);
	componentHandler.AddComponent<TransformComponent>(lightSource, Vector3D(0.0f, 0.5f, 2.0f));

	Apollo::EntityID camera = componentHandler.CreateEntity();
	componentHandler.AddComponent<CameraComponent>(camera);
}

void GameApp::Update()
{
	RZE_Game::Update();
}