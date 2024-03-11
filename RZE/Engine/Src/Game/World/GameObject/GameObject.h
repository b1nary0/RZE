#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/DebugUtils/Debug.h>

typedef U32 GameObjectID;

class GameObjectComponentBase;
class TransformComponent;

struct GameObjectStateFlags
{
	uint16_t IsInScene : 1;
	uint16_t IncludeInSave : 1;
};

class GameObject
{
public:
	typedef std::vector<GameObjectComponentBase*> ComponentList;
public:
	GameObject();
	GameObject(const std::string& name);
	~GameObject();

public:
	// @NOTE Initialize is called before any components are added to the object.
	// This is intentional. All initialization should be for the object itself.
	// The components should be self-contained state that will initialize themselves and
	// set up objects from their end.
	void Initialize();
	void Uninitialize();

public:
	template <typename TComponentType, typename... Args>
	GameObjectComponentPtr<TComponentType> AddComponent(Args... args);
	
	template <typename TComponentType>
	GameObjectComponentPtr<TComponentType> GetComponent();

	template <typename TComponentType>
	void RemoveComponent();

	void OnAddToScene();
	void OnRemoveFromScene();

	bool IncludeInSave() { return m_stateFlags.IncludeInSave; }
	void SetIncludeInSave(bool include);

	// #TODO Unsure if it's desired to update via the GameObject API but until convinced otherwise this is it
	void Update();

	GameObjectID GetID() const { return m_id; }

	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; };

	// @NOTE Should only be used by load code and editor code
	// but should really find another solution to handle this
	GameObjectComponentBase* AddComponentByID(GameObjectComponentID id);

	const ComponentList& GetComponents() const { return m_components; }

	GameObjectComponentPtr<TransformComponent> GetTransformComponent();

	bool IsInScene() { return m_stateFlags.IsInScene; }

	// Attachment
	bool IsRoot();

	void AttachTo(GameObject* gameObject);
	void DetachFromParent();

private:
	void AddChild(GameObject* child);
	void RemoveChild(GameObject* child);

public:
	void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	void Load(rapidjson::Value& data);
	
private:
	GameObjectID m_id;
	std::string m_name;

	ComponentList m_components;

	GameObjectStateFlags m_stateFlags;

	GameObject* m_parent = nullptr;
	std::vector<GameObject*> m_children;
};

template <typename TComponentType>
GameObjectComponentPtr<TComponentType> GameObject::GetComponent()
{
	// #TODO Slow function

	auto it = std::find_if(m_components.begin(), m_components.end(),
		[](const GameObjectComponentBase* component)
		{
			return TComponentType::GetID() == component->m_id;
		});

	if (it != m_components.end())
	{
		return static_cast<TComponentType*>(*it);
	}

	return nullptr;
}

template <typename TComponentType>
void GameObject::RemoveComponent()
{
	// #TODO Slow function

	auto it = std::find_if(m_components.begin(), m_components.end(),
		[](const GameObjectComponentBase* component)
		{
			return TComponentType::GetID() == component->m_id;
		});

	if (it != m_components.end())
	{
		const TComponentType* component = it;
		m_components.erase(it);

		// #TODO uninitialize here
		delete component;
	}
}

template <typename TComponentType, typename ... Args>
GameObjectComponentPtr<TComponentType> GameObject::AddComponent(Args... args)
{
	// #TODO Slow function

	auto it = std::find_if(m_components.begin(), m_components.end(),
		[](const GameObjectComponentBase* component)
		{
			return TComponentType::GetID() == component->m_id;
		});

	if (it == m_components.end())
	{
		TComponentType* component = new TComponentType(std::forward<Args>(args)...);
		m_components.push_back(component);
		component->SetOwner(this);
		component->Initialize();
		if (m_stateFlags.IsInScene)
		{
			component->OnAddToScene();
		}

		// #TODO Initialize here
		return component;
	}

	RZE_LOG_ARGS("Failed to add component %s as a component of that type already exists", TComponentType::GetComponentName());
	return nullptr;
}
