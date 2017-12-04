#pragma once

#include <Apollo/ECS/EntitySystem.h>

class RenderSystem : public Apollo::EntitySystem
{
public:
	RenderSystem();

public:
	virtual void Initialize();
	virtual void Update(std::vector<Apollo::Entity>& entities);
	virtual void ShutDown();
};