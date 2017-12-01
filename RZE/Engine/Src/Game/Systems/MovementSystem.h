#pragma once

#include <Apollo/ECS/EntitySystem.h>
#include <Apollo/ECS/EntityAdmin.h>

class MovementSystem : public IEntitySystem
{
public:
	MovementSystem(IEntityAdmin* const admin);
	virtual ~MovementSystem();

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;
};
