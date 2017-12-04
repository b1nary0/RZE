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

		void FilterEachOf(const std::vector<Entity>& entities, std::vector<Entity>& outPassedEntities)
		{
			for (auto& entity : entities)
			{
				if (entity.mComponentSet == mFilterSet)
				{
					outPassedEntities.push_back(entity);
				}
			}
		}

		void FilterAnyOf(const std::vector<Entity>& entities, std::vector<Entity>& outPassedEntities)
		{
			for (auto& entity : entities)
			{
				std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & mFilterSet;
				if (filtered.any())
				{
					outPassedEntities.push_back(entity);
				}
			}
		}

		void FilterAtLeast(const std::vector<Entity>& entities, std::vector<Entity>& outPassedEntities)
		{
			for (auto& entity : entities)
			{
				std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & mFilterSet;
				if (filtered == mFilterSet)
				{
					outPassedEntities.push_back(entity);
				}
			}
		}

		template<class TComponent>
		void AddFilterType()
		{
			ComponentTypeID componentTypeID = ComponentID<ComponentBase>::GetComponentTypeID<TComponent>();
			mFilterSet[componentTypeID] = 1;
		}

	private:
		std::bitset<ENTITY_MAX_COMPONENTS> mFilterSet;
	};
}
