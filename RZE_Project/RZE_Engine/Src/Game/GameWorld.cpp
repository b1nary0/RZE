#include "StdAfx.h"

#include "Game/ECS/EntitySystem.h"

#include "Game/GameWorld.h"

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::Update()
{
	for (auto& system : InternalGetSystems())
	{
		system->Update();
	}
}
