#pragma once

#include <vector>

#include "Utils/PrimitiveDefs.h"

class IEntityComponent;

class IEntity
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	~IEntity();

	const ComponentList& GetComponents() { return mComponents; }

	template<class TComponent>
	void AddComponent()
	{
		IEntityComponent* component = new TComponent();
		mComponents.push_back(component);
	}

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	U32 mEntityID;
	ComponentList mComponents;
};