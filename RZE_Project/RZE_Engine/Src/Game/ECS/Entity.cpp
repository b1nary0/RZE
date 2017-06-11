#include <StdAfx.h>
#include <Game/ECS/Entity.h>

#include <Game/ECS/EntityComponent.h>

IEntity::~IEntity()
{
	for (IEntityComponent* component : mComponents)
	{
		delete component;
		component = nullptr;
	}
}
