#include "GameApp.h"

#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>

#include <Windowing/WinKeyCodes.h>

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
    static float angleX = 0.0f;
    static float angleY = 0.0f;
    static float speedX = 2.5f;
    static float speedY = 2.5f;

    Functor<void, const Event&> keyEvent([this](const Event& evt)
    {
        if (evt.mInfo.mEventType == EEventType::Key)
        {
            if (evt.mKeyEvent.mKey == Win32KeyCode::Key_W)
            {
                angleX += (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX, angleY, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_S)
            {
                angleX -= (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX, angleY, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_A)
            {
                angleY -= (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX, angleY, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_D)
            {
                angleY += (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX, angleY, 0.0f)));
            }
        }
    });
    eventHandler.RegisterForEvent(EEventType::Key, keyEvent);
}

void GameApp::Start()
{
    RZE_Game::Start();

    // ALL TEST CODE
    std::vector<float> vertices = { -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f };

    const int numEntities = 1;
    for (int entIdx = 0; entIdx < numEntities; ++entIdx)
    {
        GameEntity* testEntity = GEngine->GetWorld()->AddEntity<GameEntity>();

        testEntity->AddComponent<MeshComponent>();
        MeshComponent* const meshComponent = static_cast<MeshComponent* const>(testEntity->GetComponents()[0]);

        testEntity->AddComponent<TransformComponent>();
        TransformComponent* const transformComp = static_cast<TransformComponent* const>(testEntity->GetComponents()[1]);
        transformComp->SetPosition(Vector3D(0.0f, 0.0f, -50.0f));
        transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));

        mEntities.push_back(testEntity);
    }
}

void GameApp::Update()
{
    RZE_Game::Update();

//     static float angle = 0;
//     const float speed = 1.0f;
//     angle += (1.0f / 60.0f) * speed;
// 
//     for (size_t i = 0; i < mEntities.size(); i++)
//     {
//         GameEntity* const entity = mEntities[i];
//         TransformComponent* const transfComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);
//         transfComp->SetRotation(Quaternion(Vector3D(0.0f, (i % 2 == 0) ? -angle : angle, (i % 2 == 0) ? angle : -angle)));
//     }
}
