#include "StdAfx.h"

#include "Game/GameWorld.h"
#include "Game/ECS/EntitySystem.h"
#include "Game/Systems/MovementSystem.h"
#include "Game/Systems/RenderSystem.h"

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::InitSystems()
{
	AddSystem<RenderSystem>(this);
	AddSystem<MovementSystem>(this);
}

IEntity* GameWorld::AddEntity(IEntity* const entity)
{
	InternalGetEntities().push_back(entity);
	return InternalGetEntities().back();
}

void GameWorld::Update()
{
	for (auto& system : GetSystems())
	{
		system->Update();
	}
}
