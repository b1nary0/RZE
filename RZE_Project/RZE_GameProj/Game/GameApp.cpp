#include "GameApp.h"

#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>

#include <RenderCore/Shaders/Shader.h>
#include <RenderCore/Shaders/ShaderGroup.h>

GameApp::GameApp()
	: RZE_Game()
{
}

GameApp::~GameApp()
{
}

void GameApp::RegisterEvents(EventHandler& eventHandler)
{
	Functor<void, const Event&> keyEvent([this](const Event& evt)
	{
		if (evt.mInfo.mEventType == EEventType::Key)
		{
			LOG_CONSOLE_ARGS("KeyPress: %c\n", static_cast<char>(evt.mKeyEvent.mKey));
		}
	});
	eventHandler.RegisterForEvent(EEventType::Key, keyEvent);
}

void GameApp::Start()
{
	RZE_Game::Start();

	// ALL TEST CODE

	testEntity = static_cast<GameEntity*>(GEngine->GetWorld()->AddEntity<GameEntity>());

	testEntity->AddComponent<MeshComponent>();
	MeshComponent* const meshComponent = static_cast<MeshComponent* const>(testEntity->GetComponents()[0]);
	std::vector<float> vertices = { -0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f };
	meshComponent->SetVertexList(vertices);

	testEntity->AddComponent<TransformComponent>();
	TransformComponent* const transformComp = static_cast<TransformComponent* const>(testEntity->GetComponents()[1]);
	transformComp->SetPosition(Vector3D(0.0f, 2.0f, -2.5f));
	transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 25.0f)));

	///////////////////////////////////////////////////

	testEntity2 = static_cast<GameEntity*>(GEngine->GetWorld()->AddEntity<GameEntity>());

	testEntity2->AddComponent<MeshComponent>();
	MeshComponent* const meshComponent2 = static_cast<MeshComponent* const>(testEntity2->GetComponents()[0]);
	std::vector<float> vertices2 = { -0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f };
	meshComponent2->SetVertexList(vertices2);

	testEntity2->AddComponent<TransformComponent>();
	TransformComponent* const transformComp2 = static_cast<TransformComponent* const>(testEntity2->GetComponents()[1]);
	transformComp2->SetPosition(Vector3D(0.0f, -1.0f, -12.5f));
	transformComp2->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));
}

void GameApp::Update()
{
	RZE_Game::Update();

	static float angle = 0;
	angle += 1.0f / 60.0f;

	TransformComponent* const transformComp = static_cast<TransformComponent* const>(testEntity->GetComponents()[1]);
	transformComp->SetPosition(Vector3D(-2.0f, 0.0f, -angle));

	/////////////////////////////////////////////////////

	TransformComponent* const transformComp2 = static_cast<TransformComponent* const>(testEntity2->GetComponents()[1]);
	transformComp2->SetPosition(Vector3D(2.0f, 0.0f, -angle));
	transformComp2->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, -angle)));
}
