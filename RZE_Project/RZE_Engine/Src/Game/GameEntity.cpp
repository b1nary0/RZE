#include "StdAfx.h"

#include "Game/GameEntity.h"

GameEntity::GameEntity()
{
}

GameEntity::GameEntity(const std::string& name)
	: IEntity(name)
{
}

GameEntity::~GameEntity()
{
}
