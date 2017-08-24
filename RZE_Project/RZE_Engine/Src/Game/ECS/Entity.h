#pragma once

#include <bitset>

#include <DebugUtils/Debug.h>
#include <Utils/PrimitiveDefs.h>

// 64 bits of component glory
#define ENTITY_MAX_COMPONENTS 64

class IEntityComponent;

class IEntity
{
	friend class EntityComponentFilter;

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
			LOG_CONSOLE_ARGS("Component already exists on entity %s", GetName().c_str());
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
	void Init();

private:
	U32 mEntityID;

	std::string mEntityName;

	ComponentList mComponents;
	std::bitset<ENTITY_MAX_COMPONENTS> mComponentSet;
};