#pragma once

#include <vector>

#include "Utils/PrimitiveDefs.h"

class IEntityComponent;

class IEntity
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	const ComponentList& GetComponents() { return mComponents; }

	void AddComponent(IEntityComponent* const component)
	{
		mComponents.push_back(component);
	}

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	U32 mEntityID;
	ComponentList mComponents;
};
