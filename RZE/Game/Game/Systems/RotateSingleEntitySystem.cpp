#include <Game/Systems/RotateSingleEntitySystem.h>

#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Utils/Functor.h>

#include <Game/GameApp.h>

void RotateSingleEntitySystem::Initialize()
{
	// #TODO
	// This is the place where a system may read it's data-driven settings/configuration.

	Functor<void, Apollo::EntityID> onEntityAdded([this](Apollo::EntityID entityID)
	{
		const NameComponent* const nameComponent = InternalGetEntityHandler().GetComponent<NameComponent>(entityID);
		if (nameComponent->Name == "Nyra")
		{
			mRotateEntityID = entityID;
		}
	});
	InternalGetEntityHandler().RegisterForComponentAddNotification<NameComponent>(onEntityAdded);
}

void RotateSingleEntitySystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	if (mRotateEntityID != Apollo::kInvalidEntityID)
	{
		const float RotateSpeed = 12.5f;
		const Vector3D RotateAxis(0.0f, 1.0f, 0.0f);
		const float DeltaTime = static_cast<float>(RZE_Application::RZE().GetDeltaTime());

		TransformComponent* const xformComp = InternalGetEntityHandler().GetComponent<TransformComponent>(mRotateEntityID);
		xformComp->Rotate(RotateAxis * RotateSpeed * DeltaTime);
	}
}

void RotateSingleEntitySystem::ShutDown()
{
}