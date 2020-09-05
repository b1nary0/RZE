#pragma once

#include <bitset>

#include <Apollo/ECS/Entity.h>

namespace Apollo
{
	class EntityComponentFilter
	{
	public:
		EntityComponentFilter()
		{
		}

		void FilterEachOf(const std::vector<Entity>& entities, std::vector<EntityID>& outPassedEntities) const
		{
			for (auto& entity : entities)
			{
				// #NOTE 
				// Quick optim here, better solution can be less hacky.
				// We allocate more than we need for entities at intervals,
				// so some of the entities are invalid.
				if (entity.mEntityID == Apollo::kInvalidEntityID)
				{
					continue;
				}

				if (entity.mComponentSet == mFilterSet)
				{
					outPassedEntities.push_back(entity.mEntityID);
				}
			}
		}

		void FilterAnyOf(const std::vector<Entity>& entities, std::vector<EntityID>& outPassedEntities) const
		{
			for (auto& entity : entities)
			{
				// #NOTE 
				// Quick optim here, better solution can be less hacky.
				// We allocate more than we need for entities at intervals,
				// so some of the entities are invalid.
				if (entity.mEntityID == Apollo::kInvalidEntityID)
				{
					continue;
				}

				std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & mFilterSet;
				if (filtered.any())
				{
					outPassedEntities.push_back(entity.mEntityID);
				}
			}
		}

		void FilterAtLeast(const std::vector<Entity>& entities, std::vector<EntityID>& outPassedEntities) const
		{
			for (auto& entity : entities)
			{
				// #NOTE 
				// Quick optim here, better solution can be less hacky.
				// We allocate more than we need for entities at intervals,
				// so some of the entities are invalid.
				if (entity.mEntityID == Apollo::kInvalidEntityID)
				{
					continue;
				}

				std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & mFilterSet;
				if (filtered == mFilterSet)
				{
					outPassedEntities.push_back(entity.mEntityID);
				}
			}
		}

		template<class TComponent>
		void AddFilterType()
		{
			ComponentID componentTypeID = ComponentTypeID<ComponentBase>::GetComponentTypeID<TComponent>();
			mFilterSet[componentTypeID] = 1;
		}

	private:
		std::bitset<ENTITY_MAX_COMPONENTS> mFilterSet;
	};
}
