#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <Utils/DebugUtils/Debug.h>

typedef U32 GameObjectID;

class GameObjectComponentBase;
class TransformComponent;
class Vector3D;

// The Editor's copy and paste (GameObjectSnapshot) handles each flag itself, so a new one needs a decision there
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
	// guid is fixed for the object's lifetime. GameScene creates objects and picks it: a loaded object's saved one,
	// or a new one that no other object in the scene has.
	explicit GameObject(ObjectGUID guid);
	GameObject(const std::string& name, ObjectGUID guid);
	~GameObject();

	// The ID is what GameObjectPtr tracks the object's lifetime by, so it must stay unique to one instance
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

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

	// Each asserts, then does nothing, if the object is already in (or out of) the scene, so check IsInScene first
	// where either is possible. An object can be out of the scene but still owned by it: waiting to be destroyed, or
	// set aside by a tool (the Editor's undo) to be put back later.
	void OnAddToScene();
	void OnRemoveFromScene();

	bool IncludeInSave() { return m_stateFlags.IncludeInSave; }
	void SetIncludeInSave(bool include);

	// #TODO Unsure if it's desired to update via the GameObject API but until convinced otherwise this is it
	void Update();

	// Unique within a session; see ObjectGUID for identity that lasts across sessions
	GameObjectID GetID() const { return m_id; }
	// Generated with the object, then saved and loaded with it
	ObjectGUID GetGUID() const { return m_guid; }

	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; };

	// @NOTE Should only be used by load code and editor code
	// but should really find another solution to handle this
	GameObjectComponentBase* AddComponentByID(GameObjectComponentID id);

	const ComponentList& GetComponents() const { return m_components; }
	// Lifetime-checked handles to every component, in the same order as GetComponents
	void GetComponentHandles(std::vector<GameObjectComponentPtr<GameObjectComponentBase>>& outHandles) const;

	GameObjectComponentPtr<TransformComponent> GetTransformComponent();

	bool IsInScene() { return m_stateFlags.IsInScene; }

	// Axis-aligned world-space box around every mesh on this object and its descendants; false if none have one
	bool GetHierarchyBounds(Vector3D& outMin, Vector3D& outMax);

	// Attachment
	bool IsRoot();
	// Null for a root object
	GameObjectPtr GetParent() const { return m_parent; }

	int NumChildren();
	bool HasChildren();
	GameObjectPtr GetChildAtIndex(int index);
	// child's position among this object's children, or -1 if it isn't one
	int GetChildIndex(const GameObject* child) const;
	// Whether gameObject is this object or anywhere below it in the hierarchy. Attaching this object to one of
	// those would make a cycle.
	bool IsSelfOrAncestorOf(const GameObjectPtr& gameObject) const;

	// childIndex is where it goes among the parent's children; -1 (or past the end) adds it last
	void AttachTo(GameObjectPtr gameObject, int childIndex = -1);
	void DetachFromParent();

	// This object, then its descendants, depth first
	void GetSubtree(std::vector<GameObject*>& outObjects);


private:
	void AddChild(GameObject* child, int childIndex);
	void RemoveChild(GameObject* child);

	bool GrowHierarchyBounds(Vector3D& inOutMin, Vector3D& inOutMax);

public:
	// Writes this object's entry in the scene format (SceneFormat), then each child's
	void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	// Adds the components in a scene-format entry; GameScene handles its identity and parent
	void LoadComponents(const rapidjson::Value& entry);
	
private:
	GameObjectID m_id;
	const ObjectGUID m_guid;
	std::string m_name;

	ComponentList m_components;

	GameObjectStateFlags m_stateFlags;

	GameObjectPtr m_parent = nullptr;
	std::vector<GameObjectPtr> m_children;
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
