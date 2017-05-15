#pragma once

#include "Game/ECS/EntityAdmin.h"

class GameWorld : public IEntityAdmin
{

public:
	GameWorld();
	virtual ~GameWorld();

	void InitSystems();

	virtual IEntity* AddEntity(IEntity* const entity);

	virtual void Update() override;

private:

};
