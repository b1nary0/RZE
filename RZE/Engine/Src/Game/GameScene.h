#pragma once

// @TODO When GameObjectPtr doesnt live in GameObject.h remove this include
#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <RapidJSON/fwd.h>

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

	// Main thread. Creates a game object from its serialized data and adds it to the scene.
	// A parent referenced by the data must already be in the scene.
	void DeserializeGameObject(const char* name, rapidjson::Value& data);

	void SetCurrentScenePath(const Filepath& filePath) { mCurrentScenePath = filePath; }
	const Filepath& GetCurrentScenePath() const { return mCurrentScenePath; }
	
	GameObjectPtr FindGameObjectByName(const std::string& name);
	GameObjectPtr AddGameObject(const std::string& name);
	void AddGameObject(const GameObjectPtr& gameObject);
	
	void ForEachGameObject(Functor<void, GameObjectPtr> func);
	void RemoveGameObject(GameObjectPtr& gameObject);
	
	// @NOTE Creates GameObject with TransformComponent, as all gameobjects have a spatial representation
	GameObjectPtr CreateGameObject();

private:
	// @NOTE Creates GameObject with no components. Just used for load code.
	std::unique_ptr<GameObject> CreateGameObjectNoComponents();

	void AddGameObject(std::unique_ptr<GameObject>&& gameObject);

	void InternalRemoveGameObject(GameObjectPtr& gameObject);
	void ProcessObjectRemoveDeferrals();

private:
	Filepath mCurrentScenePath;

	// #TODO Implement sparse array to make this faster
	std::vector<std::unique_ptr<GameObject>> m_objectRegistry;

	std::vector<GameObjectPtr> m_objectsToRemove;
};