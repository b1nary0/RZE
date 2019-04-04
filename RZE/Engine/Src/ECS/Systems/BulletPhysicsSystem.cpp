#include <StdAfx.h>
#include <ECS/Systems/BulletPhysicsSystem.h>

BulletPhysicsSystem::BulletPhysicsSystem(Apollo::EntityHandler* const entityHandler)
	: Apollo::EntitySystem(entityHandler)
{
}

BulletPhysicsSystem::~BulletPhysicsSystem()
{
}

void BulletPhysicsSystem::Initialize()
{
}

void BulletPhysicsSystem::Update(const std::vector<Apollo::EntityID>& entities)
{

}
