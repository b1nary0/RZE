#include <StdAfx.h>
#include <Game/Systems/InteractiveSpawningSystem.h>

#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <random>

void InteractiveSpawningSystem::Initialize()
{
	Functor<void, Apollo::EntityID> onWalkerAdded([this](Apollo::EntityID entity)
	{
		const NameComponent* const nameComponent = InternalGetEntityHandler().GetComponent<NameComponent>(entity);
		if (nameComponent->Name == "MechWalker" && mWalkerTransform == nullptr)
		{
			mWalkerTransform = InternalGetEntityHandler().GetComponent<TransformComponent>(entity);
		}
	});
	InternalGetEntityHandler().RegisterForComponentAddNotification<NameComponent>(onWalkerAdded);
}

void InteractiveSpawningSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	float deltaT = RZE_Application::RZE().GetDeltaTime();
	if (mWalkerTransform != nullptr)
	{
		mWalkerTransform->Rotate(Quaternion(Vector3D(0.0f, 25.0f, 0.0f) * deltaT));
	}
}

void InteractiveSpawningSystem::ShutDown()
{

}
