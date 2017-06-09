#include "GameApp.h"

#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>

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

	GameEntity* entity = static_cast<GameEntity*>(GEngine->GetWorld()->AddEntity(new GameEntity()));
	entity->AddComponent(new MeshComponent());
	MeshComponent* const meshComponent = static_cast<MeshComponent* const>(entity->GetComponents()[0]);
	
	std::vector<float> vertices = { -0.5f, -0.25f, 0.0f,
									0.5f, -0.5f, 0.0f,
									0.0f, 0.5f, 0.0f };
	
	meshComponent->SetVertexList(vertices);
}

void GameApp::Update()
{
	RZE_Game::Update();
}
