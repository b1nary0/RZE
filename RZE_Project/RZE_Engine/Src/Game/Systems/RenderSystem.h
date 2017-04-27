#pragma once

#include "Game/ECS/EntitySystem.h"

class RenderSystem : public IEntitySystem
{
public:
	RenderSystem();
	virtual ~RenderSystem();

	virtual void Update() override;
};
