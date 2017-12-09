#include <Game/Systems/RotateSystem.h>

#include <RZE.h>

#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MeshComponent.h>

#include <Utils/Math/Math.h>

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

	mVelocity = Vector3D(0.0f, MathUtils::ToRadians(1.0f), 0.0f);
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
