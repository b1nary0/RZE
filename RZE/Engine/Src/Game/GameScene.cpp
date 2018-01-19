#include <StdAfx.h>
#include <Game/GameScene.h>

#include <ECS/Systems/RenderSystem.h>

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize()
{
	mComponentHandler.Initialize();

	mComponentHandler.AddSystem<RenderSystem>();
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	mComponentHandler.Update();
}

void GameScene::Finish()
{
	mComponentHandler.ShutDown();
}