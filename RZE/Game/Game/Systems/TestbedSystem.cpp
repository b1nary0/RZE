#include <Game/Systems/TestbedSystem.h>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Game/GameApp.h>

void TestbedSystem::Initialize()
{
	const int maxEntities = 1000;
	GameScene& activeScene = RZE_Application::RZE().GetActiveScene();
	for (int entityCount = 0; entityCount < maxEntities; ++entityCount)
	{
		Apollo::EntityID entid = activeScene.CreateEntity("entity");
		activeScene.GetEntityHandler().AddComponent<MeshComponent>(entid);

		TransformComponent* const transfComp = activeScene.GetEntityHandler().GetComponent<TransformComponent>(entid);
		transfComp->SetPosition(Vector3D(-5.0f, 0.0f, 0.0f));
	}
}

void TestbedSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
}

void TestbedSystem::ShutDown()
{
}