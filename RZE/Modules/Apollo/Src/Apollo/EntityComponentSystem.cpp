#include <Apollo/EntityComponentSystem.h>

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
		mEntities.reserve(150);
	}

	void EntityComponentSystem::Update()
	{
	}

	void EntityComponentSystem::ShutDown()
	{
	}

}