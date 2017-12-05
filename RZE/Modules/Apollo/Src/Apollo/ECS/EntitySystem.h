#pragma once

#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponentFilter.h>

namespace Apollo
{
	struct Entity;

	class EntitySystem
	{
	public:
		EntitySystem() {};

		virtual void Initialize() = 0;
		virtual void Update(std::vector<EntityID>& entities) = 0;
		virtual void ShutDown() = 0;

		const EntityComponentFilter& GetComponentFilter() const { return mComponentFilter; }

	protected:
		EntityComponentFilter& InternalGetComponentFilter() { return mComponentFilter; }

	private:
		EntityComponentFilter mComponentFilter;
	};
}
