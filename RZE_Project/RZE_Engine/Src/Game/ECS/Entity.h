#pragma once

#include <bitset>
#include <vector>

#include "Utils/PrimitiveDefs.h"

class IEntityComponent;

class IEntity
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	IEntity();
	~IEntity();

	const ComponentList& GetComponents() { return mComponents; }

	template<class TComponent, class... Args>
	void AddComponent(Args... args)
	{
		IEntityComponent* component = new TComponent(args...);
		mComponents.push_back(component);
		mComponentSet[component->GetID()] = 1;
	}

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	U32 mEntityID;
	ComponentList mComponents;

	// 128 bits of component glory
	std::bitset<128> mComponentSet;
};