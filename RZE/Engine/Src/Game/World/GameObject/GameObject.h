#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/DebugUtils/Debug.h>

#include <unordered_map>

class GameObject
{
public:
	GameObject();
	~GameObject();

public:
	template <typename TComponentType, typename... Args>
	TComponentType* AddComponent(Args... args);

	template <typename TComponentType>
	TComponentType* GetComponent();

	template <typename TComponentType>
	void RemoveComponent();
	
private:
	std::unordered_map<U32, GameObjectComponentBase*> m_components;
};

template <typename TComponentType>
TComponentType* GameObject::GetComponent()
{
	if (m_components.count(TComponentType::GetID()) > 0)
	{
		return m_components[TComponentType::GetID()];
	}
	return nullptr;
}

template <typename TComponentType>
void GameObject::RemoveComponent()
{
	if (m_components.count(TComponentType::GetID()) > 0)
	{
		TComponentType* component = m_components[TComponentType::GetID()];
		m_components.erase(TComponentType::GetID());

		// #TODO uninitialize here
		delete component;
	}
}

template <typename TComponentType, typename ... Args>
TComponentType* GameObject::AddComponent(Args... args)
{
	if (m_components.count(TComponentType::GetID()) == 0)
	{
		TComponentType* component = new TComponentType(std::forward<Args>(args));
		m_components[TComponentType::GetID()] = component;

		// #TODO Initialize here
		return component;
	}

	LOG_CONSOLE_ARGS("Failed to add component %s as a component of that type already exists", TComponentType::GetComponentName());
	return nullptr;
}
