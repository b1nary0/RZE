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

	template<class TComponent, class... Args>
	void AddComponent(Args... args)
	{
		IEntityComponent* component = new TComponent(args...);
		mComponents.push_back(component);
	}

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	U32 mEntityID;
	ComponentList mComponents;
};