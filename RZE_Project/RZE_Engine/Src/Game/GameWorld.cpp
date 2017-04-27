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
	AddSystem<RenderSystem>();
	AddSystem<MovementSystem>();
}

void GameWorld::Update()
{
	for (auto& system : InternalGetSystems())
	{
		system->Update();
	}
}
