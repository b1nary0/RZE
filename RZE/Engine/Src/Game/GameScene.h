#pragma once

class GameObject;

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
	void Save(FilePath filePath);
	void Load(FilePath filePath);
	void Unload();
	
	std::shared_ptr<GameObject> FindGameObjectByName(const std::string& name);
	std::shared_ptr<GameObject> AddGameObject(const std::string& name);
	void RemoveGameObject(const std::shared_ptr<GameObject>& gameObject);
	
	void ForEachGameObject(Functor<void, std::shared_ptr<GameObject>> func);

private:
	// @NOTE Creates GameObject with TransformComponent, as all gameobjects have a spatial representation
	std::shared_ptr<GameObject> CreateGameObject();
	// @NOTE Creates GameObject with no components. Just used for load code.
	std::shared_ptr<GameObject> CreateGameObjectNoComponents();
	void AddGameObject(const std::shared_ptr<GameObject>& gameObject);
	
private:
	FilePath mCurrentScenePath;

	// #TODO Implement sparse array to make this faster
	std::vector<std::shared_ptr<GameObject>> m_objectRegistry;
};