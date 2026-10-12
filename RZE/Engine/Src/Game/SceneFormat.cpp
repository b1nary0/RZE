#include <StdAfx.h>
#include <Game/SceneFormat.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>

namespace
{
	ObjectGUID ReadGUID(const rapidjson::Value& entry, const char* key)
	{
		const auto member = entry.FindMember(key);
		return member != entry.MemberEnd() && member->value.IsString() ? ObjectGUIDs::FromString(member->value.GetString()) : ObjectGUIDs::k_invalid;
	}

	void SetString(rapidjson::Value& object, const char* key, const std::string& text, rapidjson::Document::AllocatorType& allocator)
	{
		rapidjson::Value value(text.c_str(), static_cast<rapidjson::SizeType>(text.size()), allocator);
		const auto member = object.FindMember(key);
		if (member != object.MemberEnd())
		{
			member->value = value;
		}
		else
		{
			object.AddMember(rapidjson::Value(key, allocator), value, allocator);
		}
	}

	// Version 1: { "gameobjects": { "<name>": { "id"?, "parent"?: "<name>", "components" } } }
	bool UpgradeFromVersion1(rapidjson::Document& document, std::string& outError)
	{
		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
		rapidjson::Value objects(rapidjson::kArrayType);

		const auto root = document.FindMember("gameobjects");
		if (root != document.MemberEnd())
		{
			if (!root->value.IsObject())
			{
				outError = "\"gameobjects\" isn't an object";
				return false;
			}

			// Names were unique in version 1; parents come before their children, so each parent's GUID is known
			// by the time a child refers to it
			std::unordered_map<std::string, std::string> guidByName;
			for (auto member = root->value.MemberBegin(); member != root->value.MemberEnd(); ++member)
			{
				if (!member->value.IsObject())
				{
					outError = std::string("Object \"") + member->name.GetString() + "\" isn't an object";
					return false;
				}

				rapidjson::Value entry(member->value, allocator);
				const std::string name = member->name.GetString();

				// Scenes saved before GUIDs get one now
				ObjectGUID guid = ReadGUID(entry, "id");
				if (guid == ObjectGUIDs::k_invalid)
				{
					guid = ObjectGUIDs::Generate();
				}
				const std::string guidText = ObjectGUIDs::ToString(guid);
				SetString(entry, "id", guidText, allocator);
				SetString(entry, "name", name, allocator);

				const auto parent = entry.FindMember("parent");
				if (parent != entry.MemberEnd())
				{
					const auto parentGUID = parent->value.IsString() ? guidByName.find(parent->value.GetString()) : guidByName.end();
					if (parentGUID == guidByName.end())
					{
						outError = "Object \"" + name + "\"'s parent isn't before it in the file";
						return false;
					}
					parent->value.SetString(parentGUID->second.c_str(), static_cast<rapidjson::SizeType>(parentGUID->second.size()), allocator);
				}

				guidByName[name] = guidText;
				objects.PushBack(entry, allocator);
			}

			document.RemoveMember("gameobjects");
		}

		document.AddMember("gameobjects", objects, allocator);
		return true;
	}
}

namespace SceneFormat
{
	void Write(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const std::vector<GameObjectPtr>& roots)
	{
		writer.StartObject();
		writer.Key("version");
		writer.Int(k_currentVersion);
		writer.Key("gameobjects");
		writer.StartArray();
		for (const GameObjectPtr& root : roots)
		{
			root->Save(writer);
		}
		writer.EndArray();
		writer.EndObject();
	}

	bool Upgrade(rapidjson::Document& document, std::string& outError)
	{
		if (!document.IsObject())
		{
			outError = "The document isn't an object";
			return false;
		}

		const auto version = document.FindMember("version");
		const int fileVersion = version != document.MemberEnd() && version->value.IsInt() ? version->value.GetInt() : 1;
		if (fileVersion > k_currentVersion)
		{
			outError = "It's version " + std::to_string(fileVersion) + ", newer than this build reads (" + std::to_string(k_currentVersion) + ")";
			return false;
		}

		if (fileVersion == 1 && !UpgradeFromVersion1(document, outError))
		{
			return false;
		}

		const auto objects = document.FindMember("gameobjects");
		if (objects == document.MemberEnd() || !objects->value.IsArray())
		{
			outError = "\"gameobjects\" is missing or isn't an array";
			return false;
		}

		for (const rapidjson::Value& entry : objects->value.GetArray())
		{
			if (!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString())
			{
				outError = "An object has no name";
				return false;
			}
		}

		if (version != document.MemberEnd())
		{
			document["version"].SetInt(k_currentVersion);
		}
		else
		{
			document.AddMember("version", k_currentVersion, document.GetAllocator());
		}
		return true;
	}

	rapidjson::Value& GetObjects(rapidjson::Document& document)
	{
		return document["gameobjects"];
	}
}

GameObjectLoader::GameObjectLoader(GameScene& scene)
	: m_scene(scene)
{
}

GameObjectPtr GameObjectLoader::Load(const rapidjson::Value& entry)
{
	GameObjectPtr parent;
	bool isParentInBatch = false;

	const ObjectGUID parentGUID = ReadGUID(entry, "parent");
	if (parentGUID != ObjectGUIDs::k_invalid)
	{
		const auto loadedParent = m_loadedBySavedGUID.find(parentGUID);
		if (loadedParent != m_loadedBySavedGUID.end())
		{
			parent = loadedParent->second;
			isParentInBatch = true;
		}
		else
		{
			// Outside the batch, e.g. pasting a copy of a child next to the original
			parent = m_scene.FindGameObjectByGUID(parentGUID);
		}
	}

	GameObjectPtr gameObject = m_scene.DeserializeGameObject(entry, parent);

	const ObjectGUID savedGUID = ReadGUID(entry, "id");
	if (savedGUID != ObjectGUIDs::k_invalid)
	{
		m_loadedBySavedGUID[savedGUID] = gameObject;
	}
	if (!isParentInBatch)
	{
		m_topLevelObjects.push_back(gameObject);
	}

	return gameObject;
}
