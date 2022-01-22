#pragma once

#include <Utils/PrimitiveDefs.h>

#include <string>

typedef U32 GameObjectComponentID;

template <typename TComponentBase>
class GameObjectComponentTypeID
{
public:
	template <class TComponentType>
	static inline GameObjectComponentID GetComponentTypeID()
	{
		static GameObjectComponentID id = sNextComponentID++;
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
	static GameObjectComponentID s_nextComponentID;
};

template <class TComponentBase>
GameObjectComponentID GameObjectComponentTypeID<TComponentBase>::s_nextComponentID = 0;

class GameObjectComponentBase
{
public:
	GameObjectComponentBase() = default;
	virtual ~GameObjectComponentBase() = default;

protected:
	GameObjectComponentID m_id;
	std::string m_componentName;
};

template <typename TComponentType>
class GameObjectComponent : public GameObjectComponentBase
{
public:
	GameObjectComponent() = default;
	virtual ~GameObjectComponent() = default;

public:
	static inline GameObjectComponentID GetID()
	{
		return GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<TComponentType>();
	}

	static inline const char* const GetComponentName()
	{
		return GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentName<TComponentType>();
	}
};