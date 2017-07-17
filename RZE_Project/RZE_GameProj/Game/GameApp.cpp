#include "GameApp.h"

#include <RZE.h>

#include <DebugUtils/Debug.h>

#include <Game/GameEntity.h>
#include <Game/Components/MeshComponent.h>
#include <Game/Components/TransformComponent.h>
#include <Game/Components/LightSourceComponent.h>

#include <Windowing/WinKeyCodes.h>

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
    
    static float speedX = 2.0f;
    static float speedY = 2.0f;
    static float speedZ = 4.0f;

    static float deltaT = (1.0f / 60.0f);

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

                GameEntity* entity = mEntities[2];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_S)
            {
                angleX_1 -= (1.0f / 60.0f) * speedX;

                GameEntity* entity = mEntities[2];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            
            if (evt.mKeyEvent.mKey == Win32KeyCode::Key_D)
            {
                angleY_1 += (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[2];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Key_A)
            {
                angleY_1 -= (1.0f / 60.0f) * speedY;

                GameEntity* entity = mEntities[2];
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(entity->GetComponents()[1]);

                transformComp->SetRotation(Quaternion(Vector3D(angleX_1, angleY_1, 0.0f)));
            }
            
            //
            // MainTestLight
            //
            if (evt.mKeyEvent.mKey == Win32KeyCode::Up)
            {
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);

                Vector3D currPos = transformComp->GetPosition();
                Vector3D newPos = currPos + Vector3D(0.0f, (2.0f * speedY) * deltaT, 0.0f);

                transformComp->SetPosition(newPos);
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Down)
            {
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);

                Vector3D currPos = transformComp->GetPosition();
                Vector3D newPos = currPos + Vector3D(0.0f, (-2.0f * speedY) * deltaT, 0.0f);

                transformComp->SetPosition(newPos);
            }
            
            if (evt.mKeyEvent.mKey == Win32KeyCode::LEFT)
            {
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);

                Vector3D currPos = transformComp->GetPosition();
                Vector3D newPos = currPos + Vector3D((-2.0f * speedX) * deltaT, 0.0f, 0.0f);

                transformComp->SetPosition(newPos);
            }
            else if (evt.mKeyEvent.mKey == Win32KeyCode::Right)
            {
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);

                Vector3D currPos = transformComp->GetPosition();
                Vector3D newPos = currPos + Vector3D((2.0f * speedX) * deltaT, 0.0f, 0.0f);

                transformComp->SetPosition(newPos);
            }

            if (evt.mKeyEvent.mKey == Win32KeyCode::Space)
            {
                TransformComponent* const transformComp = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);

                Vector3D currPos = transformComp->GetPosition();
                Vector3D newPos = currPos + Vector3D(0.0f, 0.0f, (2.0f * speedZ) * deltaT);

                transformComp->SetPosition(newPos);
            }
        }
    });
    eventHandler.RegisterForEvent(EEventType::Key, keyEvent);
}

void GameApp::Start()
{
    RZE_Game::Start();

    // ALL TEST CODE

    const char* const cubeFilePath = "./../RZE_Engine/Assets/3D/cube.obj";
    const char* const miniCooperFilePath = "./../RZE_Engine/Assets/3D/minicooper.obj";
    const char* const lampFilePath = "./../RZE_Engine/Assets/3D/Lamp.obj";

    MeshData* cubeMesh = new MeshData();
    cubeMesh->LoadFromFile(cubeFilePath);

    MeshData* miniCooperMesh = new MeshData();
    miniCooperMesh->LoadFromFile(miniCooperFilePath);

    MeshData* lampMesh = new MeshData();
    lampMesh->LoadFromFile(lampFilePath);

    CreateLight(cubeMesh);
    CreateGround(cubeMesh);
    CreateMiniCooper(miniCooperMesh);
    CreateLampObjects(lampMesh);
}

void GameApp::Update()
{
    RZE_Game::Update();
}

void GameApp::CreateLight(MeshData* const meshData)
{
    mLightEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    mLightEntity->AddComponent<MeshComponent>();
    MeshComponent* const lightMesh = static_cast<MeshComponent* const>(mLightEntity->GetComponents()[0]);
    lightMesh->SetMeshData(meshData);

    mLightEntity->AddComponent<TransformComponent>();
    TransformComponent* const lightTransform = static_cast<TransformComponent* const>(mLightEntity->GetComponents()[1]);
    lightTransform->SetPosition(Vector3D(0.0f, 10.0f, -10.0f));

    mLightEntity->AddComponent<LightSourceComponent>("MainTestLight");
    LightSourceComponent* const lightComponent = static_cast<LightSourceComponent* const>(mLightEntity->GetComponents()[2]);
    lightComponent->SetColor(Vector3D(1.0f, 1.0f, 1.0f));
    lightComponent->SetStrength(0.5f);

    mEntities.push_back(mLightEntity);
}

void GameApp::CreateGround(MeshData* const meshData)
{
    const int numEntities = 1;
    for (int entIdx = 0; entIdx < numEntities; ++entIdx)
    {
        GameEntity* testEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

        testEntity->AddComponent<MeshComponent>();
        MeshComponent* const meshComponent = static_cast<MeshComponent* const>(testEntity->GetComponents()[0]);
        meshComponent->SetMeshData(meshData);

        testEntity->AddComponent<TransformComponent>();
        TransformComponent* const transformComp = static_cast<TransformComponent* const>(testEntity->GetComponents()[1]);
        transformComp->SetPosition(Vector3D(-10.0f, -5.0f, -20.0f));
        transformComp->SetRotation(Quaternion(Vector3D(0.0f, 0.0f, 0.0f)));
        transformComp->SetScale(Vector3D(20.0f, 0.5f, 20.0f));

        mEntities.push_back(testEntity);
    }
}

void GameApp::CreateMiniCooper(MeshData* const meshData)
{
    GameEntity* miniCooperEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    miniCooperEntity->AddComponent<MeshComponent>("MiniCooperMesh");
    MeshComponent* const meshComponent = static_cast<MeshComponent* const>(miniCooperEntity->GetComponents()[0]);
    meshComponent->SetMeshData(meshData);

    miniCooperEntity->AddComponent<TransformComponent>();
    TransformComponent* const transformComp = static_cast<TransformComponent* const>(miniCooperEntity->GetComponents()[1]);
    transformComp->SetPosition(Vector3D(0.0f, -4.5f, -10.0f));
    transformComp->SetScale(Vector3D(0.05f, 0.05f, 0.05f));

    mEntities.push_back(miniCooperEntity);
}

void GameApp::CreateLampObjects(MeshData* const meshData)
{

    //
    // 1 - NW
    //
    GameEntity* lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    lampEntity->AddComponent<MeshComponent>("LampMesh");
    MeshComponent* meshComponent = static_cast<MeshComponent*>(lampEntity->GetComponents()[0]);
    meshComponent->SetMeshData(meshData);

    lampEntity->AddComponent<TransformComponent>();
    TransformComponent* transformComp = static_cast<TransformComponent*>(lampEntity->GetComponents()[1]);
    transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -15.0f));
    transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

    mEntities.push_back(lampEntity);

    //
    // 2 - NE
    //
    lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    lampEntity->AddComponent<MeshComponent>("LampMesh");
    meshComponent = static_cast<MeshComponent* const>(lampEntity->GetComponents()[0]);
    meshComponent->SetMeshData(meshData);

    lampEntity->AddComponent<TransformComponent>();
    transformComp = static_cast<TransformComponent* const>(lampEntity->GetComponents()[1]);
    transformComp->SetPosition(Vector3D(8.0f, -3.0f, -15.0f));
    transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

    mEntities.push_back(lampEntity);

    //
    // 3 - SW
    //
    lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    lampEntity->AddComponent<MeshComponent>("LampMesh");
    meshComponent = static_cast<MeshComponent* const>(lampEntity->GetComponents()[0]);
    meshComponent->SetMeshData(meshData);

    lampEntity->AddComponent<TransformComponent>();
    transformComp = static_cast<TransformComponent* const>(lampEntity->GetComponents()[1]);
    transformComp->SetPosition(Vector3D(-8.0f, -3.0f, -2.0f));
    transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

    mEntities.push_back(lampEntity);

    //
    // 4 - SE
    //
    lampEntity = RZE_Engine::Get()->GetWorld()->AddEntity<GameEntity>();

    lampEntity->AddComponent<MeshComponent>("LampMesh");
    meshComponent = static_cast<MeshComponent* const>(lampEntity->GetComponents()[0]);
    meshComponent->SetMeshData(meshData);

    lampEntity->AddComponent<TransformComponent>();
    transformComp = static_cast<TransformComponent* const>(lampEntity->GetComponents()[1]);
    transformComp->SetPosition(Vector3D(8.0f, -3.0f, -2.0f));
    transformComp->SetScale(Vector3D(0.5f, 0.5f, 0.5f));

    mEntities.push_back(lampEntity);
}
