#pragma once

#include <string>
#include <bitset>

#include <Utils/PrimitiveDefs.h>

namespace Apollo
{
	typedef U32 ComponentID;

	template <class TComponentBase>
	class ComponentTypeID
	{
	public:
		template <class TComponentType>
		static ComponentID GetComponentTypeID()
		{
			static ComponentID id = sNextComponentID++;
			return id;
		}

	private:
		static ComponentID sNextComponentID;
	};

	template <class TBase>
	ComponentID ComponentTypeID<TBase>::sNextComponentID = 0;

	struct ComponentBase
	{
		inline const std::string& GetName() const { return mName; }
		inline void SetName(const std::string& newName) { mName = newName; }

		std::string mName;
		U32 id; // Temp? Solution for removing components -- need to know which component by id to remove.
	};

	template <typename TComponentType>
	struct Component : public ComponentBase
	{
	public:
		Component()
		{
			id = GetID();
		}

		~Component() {}

		static inline U32 GetID()
		{
			return ComponentTypeID<ComponentBase>::GetComponentTypeID<TComponentType>();
		}

	private:
	};
}

