#pragma once

#include "Game/ECS/EntityAdmin.h"

class GameWorld : public IEntityAdmin
{

public:
	GameWorld();
	virtual ~GameWorld();

	void InitSystems();

	virtual void Update() override;

private:

};
