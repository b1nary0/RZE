#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <string>
#include <unordered_map>
#include <vector>

class GameScene;

// The layout scene files (and copied GameObjects) are written in, version 2:
//
//   { "version": 2,
//     "gameobjects": [ { "id": "<GUID>", "name": "Tank", "parent": "<GUID>", "components": { ... } }, ... ] }
//
// "parent" is left out for top-level objects. A parent always comes before its children. Names don't identify
// objects; GUIDs do (see ObjectGUID).
//
// Version 1 keyed "gameobjects" by name and linked parents by name; Upgrade converts it on load.
namespace SceneFormat
{
	constexpr int k_currentVersion = 2;

	// The whole document for roots and all their children
	void Write(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const std::vector<GameObjectPtr>& roots);

	// Checks a parsed document and converts an older version to the current one in place, so readers only handle the
	// current version. False, with outError set, if the document isn't a scene. Safe on any thread.
	bool Upgrade(rapidjson::Document& document, std::string& outError);

	// The "gameobjects" array of an upgraded document; empty if it has none
	rapidjson::Value& GetObjects(rapidjson::Document& document);
}

// Creates GameObjects from a batch of entries in the scene format, linking each to its parent. Parents are looked
// up by the GUID saved in the batch, so links hold even when an object is given a new GUID (its saved one was
// taken, e.g. pasting a copy of an object still in the scene); a parent that isn't in the batch is looked up in the
// scene. Entries must come parents first, as SceneFormat writes them.
class GameObjectLoader
{
public:
	explicit GameObjectLoader(GameScene& scene);

	// The created object. Main thread only.
	GameObjectPtr Load(const rapidjson::Value& entry);

	// The objects loaded so far that weren't parented to another object in the batch
	const std::vector<GameObjectPtr>& GetTopLevelObjects() const { return m_topLevelObjects; }

private:
	GameScene& m_scene;
	// The GUID each loaded object was saved with -> the object
	std::unordered_map<ObjectGUID, GameObjectPtr> m_loadedBySavedGUID;
	std::vector<GameObjectPtr> m_topLevelObjects;
};
