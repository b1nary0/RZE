#pragma once

#include "Game/ECS/EntitySystem.h"

class MovementSystem : public IEntitySystem
{
public:
	MovementSystem();
	virtual ~MovementSystem();

	virtual void Update() override;;
};
