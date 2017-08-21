#include <StdAfx.h>
#include <Game/ECS/Entity.h>

#include <Game/ECS/EntityComponent.h>

static U32 sNextEntityID = 0;

IEntity::IEntity()
{
	mEntityID = sNextEntityID++;
}

IEntity::~IEntity()
{
	for (IEntityComponent* component : mComponents)
	{
		delete component;
		component = nullptr;
	}
}
