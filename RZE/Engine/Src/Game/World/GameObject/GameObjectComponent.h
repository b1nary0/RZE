#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <string>

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

public:
	virtual void Update() = 0;

public:
	GameObjectComponentID m_id; // #TODO For now

protected:
	std::string m_componentName;
};

template <typename TComponentType>
class GameObjectComponent : public GameObjectComponentBase
{
public:
	GameObjectComponent()
	{
		m_id = GetID();
		m_componentName = GetComponentName();
	}

	virtual ~GameObjectComponent() = default;

public:
	static GameObjectComponentID GetID()
	{
		return GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<TComponentType>();
	}

	static const char* const GetComponentName()
	{
		return GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentName<TComponentType>();
	}

	void Update() override {}
};