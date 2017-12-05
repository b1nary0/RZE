#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Functor.h>

namespace Apollo
{
	class ComponentHandler;
}

class RenderSystem : public Apollo::EntitySystem
{
public:
	RenderSystem();

public:
	virtual void Initialize();
	virtual void Update(std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();
};