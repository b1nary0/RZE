#include <Apollo/EntityComponentSystem.h>

#include <Apollo/ECS/EntitySystem.h>

namespace Apollo
{

	EntityComponentSystem::EntityComponentSystem()
	{
	}

	EntityComponentSystem::~EntityComponentSystem()
	{
	}

	void EntityComponentSystem::Initialize()
	{
		mAdmin.Initialize();
	}

	void EntityComponentSystem::Update()
	{
		mAdmin.Update();
	}

	void EntityComponentSystem::ShutDown()
	{
		mAdmin.ShutDown();
	}

	void EntityComponentSystem::EntityAdminImpl::Initialize()
	{
		InternalGetEntities().reserve(150);

		for (auto& system : InternalGetSystems())
		{
			system->Initialize();
		}
	}

	void EntityComponentSystem::EntityAdminImpl::Update()
	{
		for (auto& system : InternalGetSystems())
		{
			system->Update(InternalGetEntities());
		}
	}

	void EntityComponentSystem::EntityAdminImpl::ShutDown()
	{

	}

}