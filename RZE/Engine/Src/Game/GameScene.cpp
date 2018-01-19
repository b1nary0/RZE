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
	mEntityHandler.Initialize();

	mEntityHandler.AddSystem<RenderSystem>();
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	mEntityHandler.Update();
}

void GameScene::Finish()
{
	mEntityHandler.ShutDown();
}