#include <Game/Systems/RotateSystem.h>

#include <RZE.h>

#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MeshComponent.h>

RotateSystem::RotateSystem()
{
}

RotateSystem::~RotateSystem()
{
}

void RotateSystem::Initialize()
{
	InternalGetComponentFilter().AddFilterType<TransformComponent>();

	mVelocity = Vector3D(0.0f, 0.05f, 0.0f);
}

void RotateSystem::Update(std::vector<Apollo::EntityID>& entities)
{
	Functor<void, Apollo::EntityID> TransformFunc([this](Apollo::EntityID entity)
	{
		TransformComponent* const comp = RZE_Engine::Get()->GetComponentHandler().GetComponent<TransformComponent>(entity);

		comp->Rotation = comp->Rotation + Quaternion(mVelocity);
	});

	RZE_Engine::Get()->GetComponentHandler().ForEach<TransformComponent, MeshComponent>(TransformFunc);
}

void RotateSystem::ShutDown()
{

}
