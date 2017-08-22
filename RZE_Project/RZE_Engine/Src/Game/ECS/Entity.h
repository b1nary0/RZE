#pragma once

#include <bitset>
#include <vector>

#include <DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

class IEntityComponent;

class IEntity
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	IEntity();
	IEntity(const std::string& name);
	~IEntity();

	const ComponentList& GetComponents() { return mComponents; }

	template<class TComponent, class... Args>
	TComponent* AddComponent(Args... args)
	{
		ComponentTypeID componentTypeID = ComponentID<IEntityComponent>::GetComponentTypeID<TComponent>();
		if (mComponentSet[componentTypeID])
		{
			LOG_CONSOLE_ARGS("%s already exists on entity %s", STRINGIFY(TComponent), "Test");
			return nullptr;
		}

		IEntityComponent* component = new TComponent(args...);
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

	inline const std::string& GetName() { return mEntityName; }

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

	inline void SetName(const std::string& name) { mEntityName = name; }

private:
	U32 mEntityID;

	std::string mEntityName;

	ComponentList mComponents;
	// 128 bits of component glory
	std::bitset<128> mComponentSet;
};