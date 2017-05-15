#pragma once

#include "Game/ECS/EntitySystem.h"

class MovementSystem : public IEntitySystem
{
public:
	MovementSystem(IEntityAdmin* const admin);
	virtual ~MovementSystem();

	virtual void Update() override;;
};
