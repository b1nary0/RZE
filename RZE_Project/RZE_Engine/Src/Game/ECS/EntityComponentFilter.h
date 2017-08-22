#pragma once

#include <bitset>

#include <Game/ECS/Entity.h>

class EntityComponentFilter
{
public:
	EntityComponentFilter()
	{
	}

	void FilterEachOf(const std::vector<IEntity*>& entities, std::vector<IEntity*>& outPassedEntities)
	{
		for (auto& entity : entities)
		{
			if (entity->mComponentSet == mFilterSet)
			{
				outPassedEntities.push_back(entity);
			}
		}
	}

	void FilterAnyOf(const std::vector<IEntity*>& entities, std::vector<IEntity*>& outPassedEntities)
	{
		for (auto& entity : entities)
		{
			std::bitset<16> filtered = entity->mComponentSet & mFilterSet;
			if (filtered.any())
			{
				outPassedEntities.push_back(entity);
			}
		}
	}

	void FilterAtLeast(const std::vector<IEntity*>& entities, std::vector<IEntity*>& outPassedEntities)
	{
		for (auto& entity : entities)
		{
			std::bitset<16> filtered = entity->mComponentSet & mFilterSet;
			if (filtered == mFilterSet)
			{
				outPassedEntities.push_back(entity);
			}
		}
	}

	template<class TComponent>
	void AddFilterType()
	{
		ComponentTypeID componentTypeID = ComponentID<IEntityComponent>::GetComponentTypeID<TComponent>();
		mFilterSet[componentTypeID] = 1;
	}

private:
	std::bitset<16> mFilterSet;
};
