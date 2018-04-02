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

#include <ECS/Systems/FreeCameraSystem.h>

#include <DebugUtils/DebugServices.h>

GameApp::GameApp()
	: RZE_Application()
{
}

GameApp::~GameApp()
{
}

void GameApp::Initialize()
{
	GameScene& scene = RZE_Application::RZE().GetActiveScene();

	// ALL TEST CODE
	scene.GetEntityHandler().AddSystem<FreeCameraSystem>();

	Apollo::EntityID floor = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(floor, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<TransformComponent>(floor, Vector3D(-5.0f, -5.5f, -5.0f), Quaternion(), Vector3D(10.0f, 0.5f, 10.0f));

	Apollo::EntityID lightSource = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(lightSource, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<LightSourceComponent>(lightSource, Vector3D(0.8f, 0.8f, 0.8f), 1.0f);
	scene.GetEntityHandler().AddComponent<TransformComponent>(lightSource, Vector3D(0.0f, 10.0f, 8.0f), Quaternion(), Vector3D(1.0f));

	Apollo::EntityID camera = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<TransformComponent>(camera, Vector3D(-4.0f, 8.0f, 10.0f), Quaternion(), Vector3D(1.0f));
	scene.GetEntityHandler().AddComponent<CameraComponent>(camera);
	CameraComponent* const camComp = scene.GetEntityHandler().GetComponent<CameraComponent>(camera);
	camComp->FOV = 45;
	camComp->NearCull = 0.1f;
	camComp->FarCull = 1000.0f;
	camComp->Forward = Vector3D(0.5f, -0.5f, -1.0f);
	camComp->UpDir = Vector3D(0.0f, 1.0f, 0.0f);
	camComp->AspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();

	mNanosuits.emplace_back(scene.GetEntityHandler().CreateEntity());
	Apollo::EntityID entity = mNanosuits.back();

	scene.GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Engine/Assets/3D/Nanosuit/Nanosuit.obj"));
	scene.GetEntityHandler().AddComponent<TransformComponent>(entity, Vector3D(0.0f, -5.0f, 0.0f), Quaternion(), Vector3D(0.5f));
}

void GameApp::Start()
{
	RZE_Application::Start();
}

void GameApp::Update()
{
	RZE_Application::Update();

	DebugServices::AddData(StringUtils::FormatString("%i nanosuits.", mNanosuits.size()), Vector3D(1.0f, 1.0f, 0.0f));
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
