#include "GameApp.h"

#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>
#include <Game/Components/LightSourceComponent.h>

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
    static float angleX_1 = 0.0f;
    static float angleY_1 = 0.0f;

    static float angleX_2 = 0.0f;
    static float angleY_2 = 0.0f;

    static float speedX = 4.0f;
    static float speedY = 4.0f;

    Functor<void, const Event&> keyEvent([this](const Event& evt)
    {
        if (evt.mInfo.mEventType == EEventType::Key)
        {
            //
            // First ship
            //
            if (evt.mKeyEvent.mKey == Win32KeyCode::Key_W)
            {
                angleX_1 += (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_S)
            {
                angleX_1 -= (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            
            if (evt.mKeyEvent.mKey == Win32KeyCode::Key_D)
            {
                angleY_1 += (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_A)
            {
                angleY_1 -= (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[0];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            
            //
            // Second ship
            //
            if (evt.mKeyEvent.mKey == Win32KeyCode::Up)
            {
                angleX_2 += (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[1];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_2, angleY_2, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Down)
            {
                angleX_2 -= (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[1];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_2, angleY_2, 0.0f)));
            }
            
            if (evt.mKeyEvent.mKey == Win32KeyCode::LEFT)
            {
                angleY_2 += (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[1];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_2, angleY_2, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Right)
            {
                angleY_2 -= (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[1];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_2, angleY_2, 0.0f)));
            }
        }
    });
    eventHandler.RegisterForEvent(EEventType::Key, keyEvent);
}

void GameApp::Start()
{
    RZE_Game::Start();

    // ALL TEST CODE

    const char* const shuttleFilePath = "./../RZE_Engine/Assets/3D/shuttle.obj";
    const char* const cubeFilePath = "./../RZE_Engine/Assets/3D/cube.obj";

    MeshData* shuttleMesh = new MeshData();
    shuttleMesh->LoadFromFile(shuttleFilePath);

    MeshData* cubeMesh = new MeshData();
    cubeMesh->LoadFromFile(cubeFilePath);

    const int numEntities = 2;
    for (int entIdx = 0; entIdx < numEntities; ++entIdx)
    {
        GameEntity* testEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

        testEntity->AddComponent<MeshComponent>();
        MeshComponent* const meshComponent = static_cast<MeshComponent* const>(testEntity->GetComponents()[0]);
        meshComponent->SetMeshData(shuttleMesh);

        testEntity->AddComponent<TransformComponent>();
        TransformComponent* const transformComp = static_cast<TransformComponent* const>(testEntity->GetComponents()[1]);
        transformComp->SetPosition(Vector3D(entIdx * 7.0f, entIdx * 4.0f, -25.0f));
        transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));
        transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

        mEntities.push_back(testEntity);
    }

    mLightEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    mLightEntity->AddComponent<MeshComponent>();
    MeshComponent* const lightMesh = static_cast<MeshComponent* const>(mLightEntity->GetComponents()[0]);
    lightMesh->SetMeshData(cubeMesh);

    mLightEntity->AddComponent<TransformComponent>();
    TransformComponent* const lightTransform = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);
    lightTransform->SetPosition(Vector3D(0.0f, 3.0f, -10.0f));

    mLightEntity->AddComponent<LightSourceComponent>();
    LightSourceComponent* const lightComponent = static_cast<LightSourceComponent* const>(mLightEntity->GetComponents()[0]);
    lightComponent->SetColor(Vector3D(1.0f, 1.0f, 1.0f));
    lightComponent->SetStrength(0.1f);
}

void GameApp::Update()
{
    RZE_Game::Update();
}
