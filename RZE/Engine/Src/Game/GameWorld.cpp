#include <StdAfx.h>

#include <Game/GameWorld.h>
#include <Apollo/ECS/EntitySystem.h>
 
 GameWorld::GameWorld()
 {
 }
 
 GameWorld::~GameWorld()
 {
 }
 
 void GameWorld::InitSystems()
 {
 	//@todo:josh this is dumb and purely just to get shit working. needs to be re-thought
 	for (auto& system : InternalGetSystems())
 	{
 		system->Init();
 	}
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
