#pragma once

// @TODO When GameObjectPtr doesnt live in GameObject.h remove this include
#include <Game/World/GameObject/GameObjectDefinitions.h>

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
	void Deserialize(const Filepath& filePath);
	void Unload();
	
	GameObjectPtr FindGameObjectByName(const std::string& name);
	GameObjectPtr AddGameObject(const std::string& name);
	
	void ForEachGameObject(Functor<void, GameObjectPtr> func);
	void RemoveGameObject(GameObjectPtr& gameObject);
	
private:
	// @NOTE Creates GameObject with TransformComponent, as all gameobjects have a spatial representation
	std::unique_ptr<GameObject> CreateGameObject();
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