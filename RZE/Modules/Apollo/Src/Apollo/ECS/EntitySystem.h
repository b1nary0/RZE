#pragma once

#include <vector>

#include <Apollo/ECS/Entity.h>

namespace Apollo
{
	struct Entity;

	class EntitySystem
	{
	public:
		EntitySystem() {};

		virtual void Initialize() = 0;
		virtual void Update(std::vector<Entity>& entities) = 0;
		virtual void ShutDown() = 0;
	};
}
