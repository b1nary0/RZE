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
	
private:
	void AddGameObject(GameObject& gameObject);
	void RemoveGameObject(GameObject& gameObject);
	
private:
	FilePath mCurrentScenePath;

	// #TODO Implement sparse array to make this faster
	std::vector<GameObject*> m_objectRegistry;
};