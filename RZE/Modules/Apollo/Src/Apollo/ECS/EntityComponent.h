#pragma once

#include <string>
#include <bitset>

#include <Utils/PrimitiveDefs.h>

namespace Apollo
{
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

	struct ComponentBase
	{
		inline const std::string& GetName() const { return mName; }
		inline void SetName(const std::string& newName) { mName = newName; }

		std::string mName;
	};

	template <typename TComponentType>
	struct Component : public ComponentBase
	{
	public:
		Component() {}
		~Component() {}

		static inline U32 GetID()
		{
			return ComponentID<Component>::GetComponentTypeID<TComponentType>();
		}

	private:
	};
}

