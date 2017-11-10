#pragma once

#include "Game/ECS/EntitySystem.h"

class MovementSystem : public IEntitySystem
{
public:
	MovementSystem(IEntityAdmin* const admin);
	virtual ~MovementSystem();

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;
};
