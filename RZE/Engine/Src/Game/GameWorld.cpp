#include "StdAfx.h"

#include "Game/GameWorld.h"
#include "Game/ECS/EntitySystem.h"
#include "Game/Systems/MovementSystem.h"
#include "Game/Systems/RenderSystem.h"

#include <RenderCore/Renderer.h>

GameWorld::GameWorld(RZE_Renderer* const renderer)
{
	InternalSetRenderer(renderer);
}

GameWorld::~GameWorld()
{
}

void GameWorld::InitSystems()
{
	InternalAddSystem<RenderSystem>(this);
	InternalAddSystem<MovementSystem>(this);

	//@todo:josh this is dumb and purely just to get shit working. needs to be re-thought
	for (auto& system : InternalGetSystems())
	{
		system->Init();
	}
}

RZE_Renderer* const GameWorld::GetRenderer() const
{
	return mRenderer;
}

void GameWorld::InternalSetRenderer(RZE_Renderer* const renderer)
{
	mRenderer = renderer;
}

void GameWorld::Init()
{
	InitSystems();
}

void GameWorld::Update()
{
	//#TODO need to ensure update order here... for logic > rendering etc
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
