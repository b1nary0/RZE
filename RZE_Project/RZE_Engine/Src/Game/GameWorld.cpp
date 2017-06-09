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

	//@todo:josh this is dumb and purely just to get shit working. needs to be re-thought
	for (auto& system : InternalGetSystems())
	{
		system->Init();
	}
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

void GameWorld::ShutDown()
{
	SystemList& systemList = InternalGetSystems();
	std::reverse(systemList.begin(), systemList.end());

	for (IEntitySystem* system : systemList)
	{
		system->ShutDown();
	}
}
