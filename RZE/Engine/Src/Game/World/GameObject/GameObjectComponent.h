#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <Utils/Functor.h>
#include <Utils/Math/BoundingBox.h>
#include <Utils/Platform/TypeNames.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <string>
#include <type_traits>

#define REGISTER_GAMEOBJECTCOMPONENT(ComponentType)\
{\
	GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<ComponentType>();	\
	GameObjectComponentRegistry::RegisterComponentType(ComponentType::GetID(), #ComponentType);\
	GameObjectComponentRegistry::AddComponentFactory(ComponentType::GetID(), Functor<GameObjectComponentBase*>([]() { return new ComponentType();}));\
	ComponentPropertyRegistry::Register<ComponentType>();\
}

class GameObject;
class GameObjectComponentBase;
class ResourceDependencyList;

// #TODO Would ideally like to come back to this setup
namespace GameObjectComponentRegistry
{
	typedef std::unordered_map<GameObjectComponentID, std::string> ComponentNameIDMap;

	void RegisterComponentType(GameObjectComponentID id, const char* componentName);
	void AddComponentFactory(GameObjectComponentID id, const Functor<GameObjectComponentBase*>& factoryFunc);

	// #TODO This is only for load code, basically as there is no good solution atm for creating components during load
	// so dont use this otherwise.
	GameObjectComponentBase* CreateComponentByID(GameObjectComponentID id);

	// #TODO This should only really be used by load code.
	const ComponentNameIDMap& GetAllComponentReflectData();

	// Reports the resources a component will load when deserialized from componentData, so that scene
	// loading can preload them off the main thread. Gatherers are called from worker threads: they must only
	// read componentData and write to outDependencies.
	using ResourceDependencyGatherer = void(*)(const rapidjson::Value& componentData, ResourceDependencyList& outDependencies);

	// Registration must happen at startup, before any scene loads (the registry is read from worker threads).
	void SetResourceDependencyGatherer(GameObjectComponentID id, ResourceDependencyGatherer gatherer);
	// nullptr if the component type has no gatherer.
	ResourceDependencyGatherer GetResourceDependencyGatherer(GameObjectComponentID id);
}

template <typename TComponentBase>
class GameObjectComponentTypeID
{
public:
	template <class TComponentType>
	static inline GameObjectComponentID GetComponentTypeID()
	{
		static GameObjectComponentID id = s_nextComponentID++;
		return id;
	}

	template <class TComponentType>
	static inline const char* const GetComponentName()
	{
		static const std::string componentName = TypeNames::Get(typeid(TComponentType));
		return componentName.c_str();
	}

private:
	static GameObjectComponentID s_nextComponentID;
};

template <class TComponentBase>
GameObjectComponentID GameObjectComponentTypeID<TComponentBase>::s_nextComponentID = 0;

// #TODO Not a huge fan of this setup but easiest way to poor mans reflection for component IDs
class GameObjectComponentBase
{
public:
	GameObjectComponentBase();
	virtual ~GameObjectComponentBase();

	// The instance ID is what GameObjectComponentPtr tracks the component's lifetime by, so it must stay unique to one instance
	GameObjectComponentBase(const GameObjectComponentBase&) = delete;
	GameObjectComponentBase& operator=(const GameObjectComponentBase&) = delete;

public:
	virtual void Initialize() = 0;
	virtual void OnAddToScene() = 0;
	virtual void OnRemoveFromScene() = 0;

	virtual void Update() = 0;
	virtual void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) = 0;
	virtual void Deserialize(const rapidjson::Value& data) = 0;

	// What the component takes up, in its object's own space (e.g. a mesh's box); empty for one that takes up no
	// space, as by default. Its object's hierarchy bounds are made of these (GameObject::GetHierarchyBounds), so a
	// component whose box changes calls GameObject::MarkHierarchyBoundsChanged on its owner.
	virtual BoundingBox GetLocalBounds() const { return BoundingBox(); }

public:
	// Set by the GameObject the component is added to
	void SetOwner(const GameObjectPtr& owner) { m_owner = owner; }
	// A lifetime-checked handle, like any other to a GameObject; null until the component is added to one
	const GameObjectPtr& GetOwner() const { return m_owner; }

	const std::string& GetName() { return m_componentName; }

	GameObjectComponentInstanceID GetInstanceID() const { return m_instanceID; }

public:
	GameObjectComponentID m_id; // #TODO For now

protected:
	std::string m_componentName;
	GameObjectPtr m_owner;

private:
	GameObjectComponentInstanceID m_instanceID;
};

// The base of every component type: class MyComponent final : public GameObjectComponent<MyComponent>.
// Component types must be final. The type ID comes from the type given here, so a class deriving from a component
// would get its base's ID, and lookups, saving and the registry would take it for the base.
template <typename TComponentType>
class GameObjectComponent : public GameObjectComponentBase
{
public:
	GameObjectComponent()
	{
		static_assert(std::is_final_v<TComponentType>, "Component types must be final: a subclass would share its base's type ID");
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

	void Initialize() override {}
	void OnAddToScene() override {}
	void OnRemoveFromScene() override {}

	void Update() override {}
	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override {}
	void Deserialize(const rapidjson::Value& data) override {}
};