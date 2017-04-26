#pragma once

#include "Game/ECS/EntitySystem.h"

class RenderSystem : public IEntitySystem
{
public:
	virtual ~RenderSystem();

	virtual void Update() override;
};
