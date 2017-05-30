#pragma once

#include "Game/ECS/EntitySystem.h"

class RZE_Renderer;

class RenderSystem : public IEntitySystem
{
public:
	RenderSystem(IEntityAdmin* const admin);
	virtual ~RenderSystem();

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

private:
	RZE_Renderer* mRenderer;
};
