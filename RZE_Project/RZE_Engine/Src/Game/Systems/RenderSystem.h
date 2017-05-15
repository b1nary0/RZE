#pragma once

#include "Game/ECS/EntitySystem.h"

class RenderSystem : public IEntitySystem
{
public:
	RenderSystem(IEntityAdmin* const admin);
	virtual ~RenderSystem();

	virtual void Update() override;
};
