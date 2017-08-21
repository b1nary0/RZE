#pragma once

#include <string>
#include <bitset>

#include <Utils/PrimitiveDefs.h>

template <class TComponentBase>
class ComponentID
{
public:
	template <class TComponentType>
	static U32 GetComponentTypeID()
	{
		static U32 id = sNextComponentID++;
		return id;
	}

private:
	static U32 sNextComponentID;
};

template <class TBase>
U32 ComponentID<TBase>::sNextComponentID = 0;

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

