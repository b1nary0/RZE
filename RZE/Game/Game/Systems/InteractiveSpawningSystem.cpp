#include <Game/Systems/InteractiveSpawningSystem.h>

#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>

#include <Game/GameApp.h>

#include <random>

void InteractiveSpawningSystem::Initialize()
{
	Functor<void, Apollo::EntityID> onWalkerAdded([this](Apollo::EntityID entity)
	{
		const NameComponent* const nameComponent = InternalGetEntityHandler().GetComponent<NameComponent>(entity);
		if (nameComponent->Name == "Directional Light" && mEntityTransform == nullptr)
		{
			mEntityTransform = InternalGetEntityHandler().GetComponent<TransformComponent>(entity);
		}
	});
	InternalGetEntityHandler().RegisterForComponentAddNotification<NameComponent>(onWalkerAdded);
}

void InteractiveSpawningSystem::Update(const std::vector<Apollo::EntityID>& entities)
{
	float deltaT = static_cast<float>(RZE_Application::RZE().GetDeltaTime());
	static Vector3D velocity(0.75f, 0.0f, 0.0f);

	if (mEntityTransform != nullptr)
	{
		if (mEntityTransform->Position.X() > -5.0f || mEntityTransform->Position.X() < -20.0f)
		{
			velocity *= -1;
		}

		mEntityTransform->Position += velocity * deltaT;
	}
}

void InteractiveSpawningSystem::ShutDown()
{

}
