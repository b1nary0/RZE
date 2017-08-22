#pragma once

#include <string>
#include <bitset>

#include <Utils/PrimitiveDefs.h>

typedef U32 ComponentTypeID;

template <class TComponentBase>
class ComponentID
{
public:
	template <class TComponentType>
	static ComponentTypeID GetComponentTypeID()
	{
		static ComponentTypeID id = sNextComponentID++;
		return id;
	}

private:
	static ComponentTypeID sNextComponentID;
};

template <class TBase>
ComponentTypeID ComponentID<TBase>::sNextComponentID = 0;

class IEntityComponent
{
public:
	IEntityComponent();
	~IEntityComponent();

	inline const std::string& GetName() const { return mName; }
	inline void SetName(const std::string& newName) { mName = newName; }

	inline U32 GetID() const { return mComponentID; }

protected:
	template <class TComponentType>
	void RegisterComponentID();

private:
	static U32 sNextComponentID;

	 U32 mComponentID;

	std::string mName;
};

template <class TComponentType>
void IEntityComponent::RegisterComponentID()
{
	mComponentID = ComponentID<IEntityComponent>::GetComponentTypeID<TComponentType>();
}

