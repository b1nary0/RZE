#pragma once

#include <bitset>
#include <string>
#include <typeinfo>

#include <Apollo/ECS/Entity.h>

#include <Utils/PrimitiveDefs.h>

// This is ugly here but next step toward better saving.
// Will (should) go away when I stop being lazy
#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>

namespace Apollo
{
	typedef U32 ComponentID;

	template <class TComponentBase>
	class ComponentTypeID
	{
	public:
		template <class TComponentType>
		static inline ComponentID GetComponentTypeID()
		{
			static ComponentID id = sNextComponentID++;
			return id;
		}

		template <class TComponentType>
		static inline const char* const GetComponentName()
		{
			static std::string componentName = typeid(TComponentType).name();
			componentName = componentName.substr(componentName.find_first_of(' ') + 1, componentName.size());
			return componentName.c_str();
		}

	private:
		static ComponentID sNextComponentID;
	};

	template <class TBase>
	ComponentID ComponentTypeID<TBase>::sNextComponentID = 0;

	struct ComponentBase
	{
		U32 id; // Temp? Solution for removing components -- need to know which component by id to remove.
		std::string ComponentName;

		virtual void OnEditorInspect(Apollo::EntityID entityID) {}

		virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {}
		virtual void Load(const rapidjson::Value& data) {}
	};

	template <typename TComponentType>
	struct Component : public ComponentBase
	{
	public:
		Component()
		{
			id = GetID();
			ComponentName = GetComponentName();
		}

		~Component() {}

		static inline U32 GetID()
		{
			return ComponentTypeID<ComponentBase>::GetComponentTypeID<TComponentType>();
		}

		static inline const char* const GetComponentName()
		{
			return ComponentTypeID<ComponentBase>::GetComponentName<TComponentType>();
		}

	private:
	};
}

