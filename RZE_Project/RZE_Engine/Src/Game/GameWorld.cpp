#include "StdAfx.h"

#include "Game/GameWorld.h"
#include "Game/ECS/EntitySystem.h"
#include "Game/Systems/MovementSystem.h"
#include "Game/Systems/RenderSystem.h"

#include <RenderCore/Renderer.h>

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
	if (mRenderer)
	{
		delete mRenderer;
	}
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

RZE_Renderer* const GameWorld::GetRenderer() const
{
	return mRenderer;
}

void GameWorld::Init()
{
	mRenderer = new RZE_Renderer();

	InitSystems();
}

void GameWorld::Update()
{
	for (auto& system : GetSystems())
	{
		system->Update();
	}
}
