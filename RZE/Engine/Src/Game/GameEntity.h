#pragma once

#include <Game/ECS/Entity.h>

class GameEntity : public IEntity
{
public:
	GameEntity();
	GameEntity(const std::string& name);
	virtual ~GameEntity();

private:


};