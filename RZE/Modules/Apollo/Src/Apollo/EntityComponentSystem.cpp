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
	}

	void EntityComponentSystem::EntityAdminImpl::Update()
	{
	}

	void EntityComponentSystem::EntityAdminImpl::ShutDown()
	{

	}

}