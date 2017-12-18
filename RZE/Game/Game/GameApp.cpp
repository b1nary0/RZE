#include "GameApp.h"

#include <RZE_Config.h>
#include <RZE.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>
#include <Utils/Platform/FilePath.h>

// TEST
#include <Diotima/Graphics/Texture2D.h>

#include <Ecs/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <Game/Systems/RotateSystem.h>

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

	Apollo::EntityID bgEnt = componentHandler.CreateEntity();
	componentHandler.AddComponent<MeshComponent>(bgEnt, FilePath("Engine/Assets/3D/Quad.obj"));
	componentHandler.AddComponent<MaterialComponent>(bgEnt, FilePath("Engine/Assets/2D/Background.jpg"));

	MaterialComponent* const matComp = componentHandler.GetComponent<MaterialComponent>(bgEnt);
	Diotima::GFXTexture2D* const texture = RZE_Engine::Get()->GetResourceHandler().GetResource<Diotima::GFXTexture2D>(matComp->Texture);
	
	componentHandler.AddComponent<TransformComponent>(bgEnt, Vector3D(0.0f, 0.0f, -10.0f), Quaternion(Vector3D(0.0f, MathUtils::ToRadians(180.0f), 0.0f)), Vector3D(30.0, 20.0f, 5.0f));

	// Leaving the for loop for testing purposes
	for (int i = 0; i < 1; ++i)
	{
		Apollo::EntityID entity = componentHandler.CreateEntity();

		componentHandler.AddComponent<MeshComponent>(entity, FilePath("Engine/Assets/3D/NanoSuit.obj"));
		componentHandler.AddComponent<TransformComponent>(entity, Vector3D(), Quaternion(), Vector3D(0.5f));
		componentHandler.AddComponent<MaterialComponent>(entity, FilePath("Engine/Assets/2D/Container.jpg"));
	}

	Apollo::EntityID lightSource = componentHandler.CreateEntity();
	componentHandler.AddComponent<LightSourceComponent>(lightSource, Vector3D(0.8f, 0.8f, 0.8f), 1.0f);
	componentHandler.AddComponent<TransformComponent>(lightSource, Vector3D(0.0f, 0.5f, 2.0f));

	Apollo::EntityID camera = componentHandler.CreateEntity();
	componentHandler.AddComponent<CameraComponent>(camera);
	CameraComponent* const camComp = componentHandler.GetComponent<CameraComponent>(camera);
	camComp->Position = Vector3D(0.0f, 0.0f, 8.0f);
	camComp->FOV = 45;
	camComp->NearCull = 0.1f;
	camComp->FarCull = 1000.0f;
	camComp->Forward = Vector3D(0.0f, 0.0f, -1.0f);
	camComp->UpDir = Vector3D(0.0f, 1.0f, 0.0f);
	camComp->AspectRatio = RZE_Engine::Get()->GetWindowSize().X() / RZE_Engine::Get()->GetWindowSize().Y();
}

void GameApp::Update()
{
	RZE_Game::Update();
}