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
		if (nameComponent->Name == "Nanosuit")
		{
			mRenderEntity = entity;
		}
	});
	InternalGetEntityHandler().RegisterForComponentAddNotification<NameComponent>(onEntityAdded);
}

void TestbedSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	if (mRenderEntity != Apollo::kInvalidEntityID)
	{
		const float RotateSpeed = 50.0f;
		const Vector3D RotateAxis(0.0f, 1.0f, 0.0f);
		const float DeltaTime = static_cast<float>(RZE_Application::RZE().GetDeltaTime());

		TransformComponent* const xformComp = InternalGetEntityHandler().GetComponent<TransformComponent>(mRenderEntity);
		xformComp->Rotate(RotateAxis * RotateSpeed * DeltaTime);
	}
}

void TestbedSystem::ShutDown()
{
}