#pragma once

#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponentFilter.h>

namespace Apollo
{
	struct Entity;

	class EntityHandler;

	class EntitySystem
	{
	private:
		EntitySystem();

	public:

		EntitySystem(EntityHandler* const entityHandler)
			: mEntityHandler(entityHandler)
		{}

		virtual void Initialize() = 0;
		virtual void Update(const std::vector<Apollo::EntityID>& entities) = 0;
		virtual void ShutDown() = 0;

		const EntityComponentFilter& GetComponentFilter() const { return mComponentFilter; }

	protected:
		EntityHandler& InternalGetEntityHandler()
		{
			AssertNotNull(mEntityHandler);
			return *mEntityHandler;
		}

		EntityComponentFilter& InternalGetComponentFilter() { return mComponentFilter; }

	private:
		EntityHandler* mEntityHandler;
		EntityComponentFilter mComponentFilter;
	};
}
