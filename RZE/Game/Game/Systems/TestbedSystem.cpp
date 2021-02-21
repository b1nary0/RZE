#include <Game/Systems/TestbedSystem.h>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Utils/PrimitiveDefs.h>

#include <Game/GameApp.h>

void TestbedSystem::Initialize()
{
	Functor<void, Apollo::EntityID> onEntityAdded([this](Apollo::EntityID entity)
	{
		const NameComponent* const nameComponent = InternalGetEntityHandler().GetComponent<NameComponent>(entity);
		if (nameComponent->Name == "Plane")
		{
			mPlaneEntity = entity;
		}
	});
	InternalGetEntityHandler().RegisterForComponentAddNotification<NameComponent>(onEntityAdded);
}

void TestbedSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	const Vector3D kMaxDistances(0.0f, 0.0f, 10.0f);
	const float kMoveSpeed = 4.0f;

	GameScene& gameScene = RZE_Application::RZE().GetActiveScene();
	const float kDeltaT = static_cast<float>(RZE_Application::RZE().GetDeltaTime());

	if (mPlaneEntity != Apollo::kInvalidEntityID)
	{
		TransformComponent* const tComponent = gameScene.GetEntityHandler().GetComponent<TransformComponent>(mPlaneEntity);
		AssertNotNull(tComponent);

		static bool bMovingFoward = true;
		if (tComponent->Position.Z() > kMaxDistances.Z() || tComponent->Position.Z() < -kMaxDistances.Z())
		{
			bMovingFoward = !bMovingFoward;
			tComponent->Rotate(Vector3D(0.0f, 180.0f, 0.0f));
		}

		const float posDelta = (kMoveSpeed * kDeltaT);
		if (bMovingFoward)
		{
			tComponent->Position.SetZ(tComponent->Position.Z() + posDelta);
		}
		else
		{
			tComponent->Position.SetZ(tComponent->Position.Z() - posDelta);
		}
	}
}

void TestbedSystem::ShutDown()
{
}