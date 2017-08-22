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
	TComponent* AddComponent(Args... args)
	{
		IEntityComponent* component = new TComponent(args...);

		ComponentTypeID componentTypeID = component->GetID();
		if (componentTypeID >= mComponents.size())
		{
			mComponents.resize(mComponents.size() * 2, nullptr);
		}

		mComponents[componentTypeID] = component;
		mComponentSet[componentTypeID] = 1;

		return static_cast<TComponent*>(component);
	}

	template<class TComponent>
	TComponent* GetComponent()
	{
		TComponent* retComponent = nullptr;

		ComponentTypeID componentTypeID = ComponentID<IEntityComponent>::GetComponentTypeID<TComponent>();
		if (mComponentSet[componentTypeID])
		{
			AssertExpr(componentTypeID <= mComponents.size());
			retComponent = static_cast<TComponent*>(mComponents[componentTypeID]);

			return retComponent;
		}

		return retComponent;
	}

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	U32 mEntityID;

	ComponentList mComponents;
	// 128 bits of component glory
	std::bitset<128> mComponentSet;
};