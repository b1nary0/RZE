#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Functor.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <string>

#define REGISTER_GAMEOBJECTCOMPONENT(ComponentType)\
{\
	GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<ComponentType>();	\
	GameObjectComponentRegistry::RegisterComponentType(ComponentType::GetID(), #ComponentType);\
	GameObjectComponentRegistry::AddComponentFactory(ComponentType::GetID(), Functor<GameObjectComponentBase*>([]() { return new ComponentType();}));\
}

class GameObject;
class GameObjectComponentBase;

// @NOTE
// This stuff doesn't support class hierarchies, and will only ever use the first registered component
// in any hierarchy. Needs to be refactored to support this. As such, things like CameraComponent
// are broken because I refactored EditorCameraComponent to inherit from CameraComponent; this
// means we can't have both a CameraComponent and EditorCameraComponent reflected because the ID will
// be the same, and the ID is the driving factor behind all recognition of unique classes.

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
		static std::string componentName = typeid(TComponentType).name();
		componentName = componentName.substr(componentName.find_first_of(' ') + 1, componentName.size());
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
	GameObjectComponentBase() = default;
	virtual ~GameObjectComponentBase() = default;

public:
	virtual void Initialize() = 0;
	virtual void OnAddToScene() = 0;
	virtual void OnRemoveFromScene() = 0;

	virtual void Update() = 0;
	virtual void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) = 0;
	virtual void Deserialize(const rapidjson::Value& data) = 0;

	virtual void OnEditorInspect() = 0;

public:
	void SetOwner(GameObject* owner) { m_owner = owner; }
	GameObject* GetOwner() { return m_owner; }

	const std::string& GetName() { return m_componentName; }

public:
	GameObjectComponentID m_id; // #TODO For now

protected:
	std::string m_componentName;
	GameObject* m_owner;
};

// @NOTE:
// Pretty sure this is where the "reflection" system is failing for inherited types like
// EditorCameraComponent : CameraComponent : GameObjectComponent<CameraComponent>.
// Basically, GameObjectComponent<EditorCameraComponent> never gets called during runtime to fetch
// the id because ::GetID underneath is static to ::GetComponentTypeID<CameraComponent> even though
// we've technically registered an ID for EditorCameraComponent. We just never fetch it properly.
// No matter what, we call GameObjectComponent<EditorCameraComponent>'s version of ::GetComponentTypeID<>
// because it's the first one that gets registered in that component hierarchy.
// Feels like we need to disconnect the ID system from GameObjectComponent. Not exactly sure how atm
// but we can't be driving the ID from functions templated to the type inheriting from GameObjectComponent
// because it's totally reasonable to have a component B want to extend the behaviour of Component A
// without having to re-declare itself as a GameObjectComponent.
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

	void Initialize() override {}
	void OnAddToScene() override {}
	void OnRemoveFromScene() override {}

	void Update() override {}
	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override {}
	void Deserialize(const rapidjson::Value& data) override {}

	void OnEditorInspect() override {}
	
};