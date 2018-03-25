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
#include <Game/Systems/WeirdTempInputSystem.h>

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

	GameScene& scene = RZE_Engine::Get()->GetActiveScene();

	// ALL TEST CODE
	scene.GetEntityHandler().AddSystem<WeirdTempInputSystem>();
	scene.GetEntityHandler().AddSystem<FreeCameraSystem>();
	
	Apollo::EntityID floor = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(floor, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<TransformComponent>(floor, Vector3D(-5.0f, -5.5f, -5.0f), Quaternion(), Vector3D(10.0f, 0.5f, 10.0f));

	Apollo::EntityID lightSource = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(lightSource, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<LightSourceComponent>(lightSource, Vector3D(0.8f, 0.8f, 0.8f), 1.0f);
	scene.GetEntityHandler().AddComponent<TransformComponent>(lightSource, Vector3D(0.0f, 10.0f, 8.0f), Quaternion(), Vector3D(1.0f));

	Apollo::EntityID camera = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<TransformComponent>(camera, Vector3D(-4.0f, 15.0f, 50.0f), Quaternion(), Vector3D(1.0f));
	scene.GetEntityHandler().AddComponent<CameraComponent>(camera);
	CameraComponent* const camComp = scene.GetEntityHandler().GetComponent<CameraComponent>(camera);
	camComp->FOV = 45;
	camComp->NearCull = 0.1f;
	camComp->FarCull = 1000.0f;
	camComp->Forward = Vector3D(0.5f, -0.5f, -1.0f);
	camComp->UpDir = Vector3D(0.0f, 1.0f, 0.0f);
	camComp->AspectRatio = RZE_Engine::Get()->GetWindowSize().X() / RZE_Engine::Get()->GetWindowSize().Y();

	InputHandler::KeyActionFunc tempKeyFunc([this, &scene](const InputKey& key)
	{
		static float nextXpos = 0.0f;

		if (key.GetKeyCode() == Win32KeyCode::Key_5)
		{
			if (mNanosuits.size())
			{
				scene.GetEntityHandler().DestroyEntity(mNanosuits.back());
				mNanosuits.erase(mNanosuits.end() - 1);

				if (mNanosuits.size() == 0)
				{
					nextXpos = 0.0f;
				}
			}
		}
		else if (key.GetKeyCode() == Win32KeyCode::Key_6)
		{
			mNanosuits.emplace_back(scene.GetEntityHandler().CreateEntity());
			Apollo::EntityID entity = mNanosuits.back();

			scene.GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Engine/Assets/3D/Nanosuit/Nanosuit.obj"));
			scene.GetEntityHandler().AddComponent<TransformComponent>(entity, Vector3D(nextXpos, -5.0f, 0.0f), Quaternion(), Vector3D(0.5f));

			nextXpos += 5.0f;
		}
	});
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_5, EButtonState::ButtonState_Pressed, tempKeyFunc);
	RZE_Engine::Get()->GetInputHandler().BindAction(Win32KeyCode::Key_6, EButtonState::ButtonState_Pressed, tempKeyFunc);
}

void GameApp::Update()
{
	RZE_Game::Update();
}