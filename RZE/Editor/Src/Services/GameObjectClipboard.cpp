#include <Services/GameObjectClipboard.h>

#include <Game/World/GameObject/GameObject.h>

#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <algorithm>

namespace Editor
{
	void GameObjectClipboard::Copy(const std::vector<GameObjectPtr>& gameObjects)
	{
		std::string contents = Serialize(gameObjects);
		// Keep what was there if nothing could be copied, rather than clearing it
		if (!contents.empty())
		{
			m_contents = std::move(contents);
		}
	}

	std::string GameObjectClipboard::Serialize(const std::vector<GameObjectPtr>& gameObjects)
	{
		// GameObject::Save writes an object's children after it, so a selected object under another selected object
		// is already copied with it
		std::vector<GameObjectPtr> roots;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			if (gameObject == nullptr || !gameObject->IncludeInSave())
			{
				continue;
			}

			const bool hasCopiedAncestor = std::any_of(gameObjects.begin(), gameObjects.end(),
				[&gameObject](const GameObjectPtr& other)
				{
					return other != gameObject && other != nullptr && other->IsSelfOrAncestorOf(gameObject);
				});
			if (!hasCopiedAncestor)
			{
				roots.push_back(gameObject);
			}
		}

		if (roots.empty())
		{
			return std::string();
		}

		// The same layout as a scene file
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		writer.StartObject();
		writer.String("gameobjects");
		writer.StartObject();
		for (const GameObjectPtr& root : roots)
		{
			root->Save(writer);
		}
		writer.EndObject();
		writer.EndObject();

		return std::string(buffer.GetString(), buffer.GetSize());
	}
}
