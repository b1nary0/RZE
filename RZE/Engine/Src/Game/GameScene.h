#pragma once

#include <string>
#include <vector>

#include <Apollo/EntityHandler.h>

class GameScene
{
public:
	// #TODO(Josh) Temp. First passing it.
	struct SceneEntryTemp
	{
		Apollo::EntityID ID;
		std::string Name;
	};

public:
	GameScene();
	~GameScene();

public:
	virtual void Start();
	virtual void Update();
	virtual void ShutDown();

	Apollo::EntityHandler& GetEntityHandler() { return mEntityHandler; }

	void Initialize();

	void NewScene();
	void Save(FilePath filePath);
	void Load(FilePath filePath);
	void Unload();

	Apollo::EntityID CreateEntity(const std::string& name);
	void DestroyEntity(Apollo::EntityID entity);

	const std::vector<SceneEntryTemp>& GetSceneEntries() { return mEntityEntries; }

private:
	void AddToScene(Apollo::EntityID entityID, const std::string& name);

	void Clear();

private:
	Apollo::EntityHandler mEntityHandler;

private:
	FilePath mCurrentScenePath;

	std::vector<SceneEntryTemp> mEntityEntries;
};