#pragma once

#include "Game/ECS/EntityAdmin.h"

class GameWorld : public IEntityAdmin
{

public:
	GameWorld();
	~GameWorld();

	virtual void Update() override;

private:

};
