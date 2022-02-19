#pragma once

class GameObject;

class GameScene
{
public:
	GameScene();
	~GameScene();

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

	void ForEachGameObject(Functor<void, std::shared_ptr<GameObject>> func);

private:
	std::shared_ptr<GameObject> CreateGameObject();
	void AddGameObject(const std::shared_ptr<GameObject>& gameObject);
	void RemoveGameObject(const std::shared_ptr<GameObject>& gameObject);
	
private:
	FilePath mCurrentScenePath;

	// #TODO Implement sparse array to make this faster
	std::vector<std::shared_ptr<GameObject>> m_objectRegistry;
};