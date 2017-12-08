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
	InternalGetComponentFilter().AddFilterType<MeshComponent>();

	mVelocity = Vector3D(0.0f, 0.005f, 0.0f);
}

void RotateSystem::Update(std::vector<Apollo::EntityID>& entities)
{
	for (auto& entity : entities)
	{
		TransformComponent* const comp = RZE_Engine::Get()->GetComponentHandler().GetComponent<TransformComponent>(entity);
		comp->Rotation *= Quaternion(mVelocity);
	}
}

void RotateSystem::ShutDown()
{
}
