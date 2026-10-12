#pragma once

// @TODO When GameObjectPtr doesnt live in GameObject.h remove this include
#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <RapidJSON/fwd.h>

#include <Utils/Math/Vector3D.h>

#include <unordered_map>

class GameScene final
{
public:
	GameScene();
	virtual ~GameScene();

public:
	virtual void Start();
	virtual void Update();
	virtual void ShutDown();

	void Initialize();

	void NewScene();
	void Serialize(const Filepath& filePath);
	// Synchronous load. See SceneLoadOperation for the asynchronous equivalent.
	void Deserialize(const Filepath& filePath);
	void Unload();

	static const Filepath& GetDefaultScenePath();

	// Reads and parses a scene file without touching any scene state. Safe to call from any thread.
	static bool ParseSceneFile(const Filepath& filePath, rapidjson::Document& outDocument, std::string& outError);

	// Main thread. Creates a game object from its entry in the scene format (SceneFormat), under parent if there is
	// one, and adds it to the scene. Its parent link in the entry is ignored; GameObjectLoader resolves those.
	GameObjectPtr DeserializeGameObject(const rapidjson::Value& entry, GameObjectPtr parent);

	void SetCurrentScenePath(const Filepath& filePath) { mCurrentScenePath = filePath; }
	const Filepath& GetCurrentScenePath() const { return mCurrentScenePath; }
	
	GameObjectPtr FindGameObjectByName(const std::string& name);
	// Null if no object in the scene has guid
	GameObjectPtr FindGameObjectByGUID(ObjectGUID guid);
	GameObjectPtr AddGameObject(const std::string& name);
	void AddGameObject(const GameObjectPtr& gameObject);
	
	void ForEachGameObject(Functor<void, GameObjectPtr> func);
	void RemoveGameObject(GameObjectPtr& gameObject);
	
	// @NOTE Creates GameObject with TransformComponent, as all gameobjects have a spatial representation
	GameObjectPtr CreateGameObject();

	// Center of the bounding box around every rendered root object's position. Uses positions rather
	// than mesh bounds so it's valid while meshes are still streaming in. Returns the origin if nothing is rendered.
	Vector3D CalculateSceneCenter() const;

private:
	// @NOTE Creates GameObject with no components. Just used for load code.
	std::unique_ptr<GameObject> CreateGameObjectNoComponents(ObjectGUID guid);

	// The GUID saved in a loaded object's data ("id") if it's valid and not taken, otherwise a new one
	ObjectGUID ChooseGUID(const rapidjson::Value& data) const;
	// A new GUID that no object in the scene has
	ObjectGUID GenerateFreeGUID() const;

	void AddGameObject(std::unique_ptr<GameObject>&& gameObject);

	void InternalRemoveGameObject(GameObjectPtr& gameObject);
	void ProcessObjectRemoveDeferrals();

private:
	Filepath mCurrentScenePath;

	// #TODO Implement sparse array to make this faster
	std::vector<std::unique_ptr<GameObject>> m_objectRegistry;

	std::vector<GameObjectPtr> m_objectsToRemove;

	// Every object in m_objectRegistry by GUID, including ones waiting to be removed; GUIDs never change, so this
	// only changes when objects are added or erased
	std::unordered_map<ObjectGUID, GameObject*> m_objectsByGUID;
};