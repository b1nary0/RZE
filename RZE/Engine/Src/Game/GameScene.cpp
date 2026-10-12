#include <StdAfx.h>
#include <Game/GameScene.h>

#include <Game/SceneFormat.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <Utils/DebugUtils/Debug.h>

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize()
{
}

void GameScene::NewScene()
{
	Deserialize(GetDefaultScenePath());
}

const Filepath& GameScene::GetDefaultScenePath()
{
	static const Filepath s_defaultScenePath("Assets/Scenes/Default.scene");
	return s_defaultScenePath;
}

void GameScene::Serialize(const Filepath& filePath)
{
	std::vector<GameObjectPtr> roots;
	for (auto& gameObject : m_objectRegistry)
	{
		if (gameObject->IsInScene() && gameObject->IncludeInSave() && gameObject->IsRoot())
		{
			roots.push_back(GameObjectPtr(gameObject.get()));
		}
	}

	rapidjson::StringBuffer buf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
	SceneFormat::Write(writer, roots);
	
	File sceneFile;
	if (filePath.IsValid())
	{
		sceneFile.SetFilePath(filePath.GetRelativePath());
		mCurrentScenePath = filePath;
	}
	else
	{
		sceneFile.SetFilePath(mCurrentScenePath.GetRelativePath());
	}

	sceneFile.Open(File::EFileOpenMode::Write);
	sceneFile.Write(buf.GetString());
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();
}

void GameScene::Deserialize(const Filepath& filePath)
{
	mCurrentScenePath = filePath;

	rapidjson::Document sceneDoc;
	std::string error;
	if (!ParseSceneFile(filePath, sceneDoc, error))
	{
		RZE_LOG_ARGS("Failed to load scene: %s", error.c_str());
		return;
	}

	GameObjectLoader loader(*this);
	for (const rapidjson::Value& entry : SceneFormat::GetObjects(sceneDoc).GetArray())
	{
		loader.Load(entry);
	}
}

bool GameScene::ParseSceneFile(const Filepath& filePath, rapidjson::Document& outDocument, std::string& outError)
{
	if (!filePath.IsValid() || !filePath.Exists())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] does not exist.";
		return false;
	}

	File sceneFile(filePath);
	sceneFile.Read();
	sceneFile.Close();

	if (sceneFile.Content().empty())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] could not be read or is empty.";
		return false;
	}

	outDocument.Parse(sceneFile.Content().c_str());
	if (outDocument.HasParseError() || !outDocument.IsObject())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] is not valid JSON (error at offset " + std::to_string(outDocument.GetErrorOffset()) + ").";
		return false;
	}

	std::string formatError;
	if (!SceneFormat::Upgrade(outDocument, formatError))
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] can't be read: " + formatError + ".";
		return false;
	}

	return true;
}

GameObjectPtr GameScene::DeserializeGameObject(const rapidjson::Value& entry, GameObjectPtr parent)
{
	std::unique_ptr<GameObject> gameObject = CreateGameObjectNoComponents(ChooseGUID(entry));
	gameObject->SetName(entry["name"].GetString());
	gameObject->LoadComponents(entry);
	if (parent != nullptr)
	{
		gameObject->AttachTo(parent);
	}

	GameObjectPtr created(gameObject.get());
	AddGameObject(std::move(gameObject));
	return created;
}

std::unique_ptr<GameObject> GameScene::CreateGameObjectNoComponents(ObjectGUID guid)
{
	return std::make_unique<GameObject>(guid);
}

ObjectGUID GameScene::ChooseGUID(const rapidjson::Value& data) const
{
	// Scenes saved before GUIDs have no "id"; a pasted copy of an object still in the scene has a taken one
	const auto id = data.FindMember("id");
	const ObjectGUID saved = id != data.MemberEnd() && id->value.IsString() ? ObjectGUIDs::FromString(id->value.GetString()) : ObjectGUIDs::k_invalid;
	if (saved != ObjectGUIDs::k_invalid && m_objectsByGUID.find(saved) == m_objectsByGUID.end())
	{
		return saved;
	}

	return GenerateFreeGUID();
}

ObjectGUID GameScene::GenerateFreeGUID() const
{
	// A clash between random 64-bit values is vanishingly unlikely, but cheap to rule out
	ObjectGUID guid = ObjectGUIDs::Generate();
	while (m_objectsByGUID.find(guid) != m_objectsByGUID.end())
	{
		guid = ObjectGUIDs::Generate();
	}
	return guid;
}

void GameScene::Unload()
{
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		gameObject->OnRemoveFromScene();
		gameObject->Uninitialize();
	}

	m_objectRegistry.clear();
	m_objectsByGUID.clear();
}

void GameScene::AddGameObject(std::unique_ptr<GameObject>&& gameObject)
{
	// #TODO Slow function

	const auto it = std::find_if(m_objectRegistry.begin(), m_objectRegistry.end(), 
		[&gameObject](const auto& registryObject)
		{
			return gameObject == registryObject || registryObject->GetName() == gameObject->GetName();
		});

	AssertMsg(it == m_objectRegistry.end(), "GameObject already exists in scene");
	if (it == m_objectRegistry.end())
	{
		// GameScene only creates objects with free GUIDs (ChooseGUID, GenerateFreeGUID), so a clash here is a bug
		const bool isGUIDFree = m_objectsByGUID.emplace(gameObject->GetGUID(), gameObject.get()).second;
		AssertMsg(isGUIDFree, "A GameObject with this GUID is already in the scene");

		gameObject->OnAddToScene();
		m_objectRegistry.emplace_back(std::move(gameObject));
	}
}

void GameScene::AddGameObject(const GameObjectPtr& gameObject)
{
	AddGameObject(std::unique_ptr<GameObject>(gameObject.m_ptr));
}

void GameScene::InternalRemoveGameObject(GameObjectPtr& gameObject)
{
	const auto iter = std::find_if(m_objectRegistry.begin(), m_objectRegistry.end(),
		[&gameObject](const std::unique_ptr<GameObject>& other)
		{
			return gameObject.m_ptr == other.get();
		});
	AssertMsg(iter != m_objectRegistry.end(), "GameObject doesn't exist in scene");

	if (iter != m_objectRegistry.end())
	{
		AssertExpr(!gameObject->IsInScene());

		// @TODO This shouldnt be here but we'll end up with "leaking" objects when
		// we remove from the scene but dont call Uninitialize (which stray resources will be stale and bad)
		// which is actually more indicative of a larger dumb but i can only focus on so much dumb at a time
		// and this entire codebase is atrocious.
		gameObject->Uninitialize();
		m_objectsByGUID.erase(gameObject->GetGUID());

		if (m_objectRegistry.size() > 1)
		{
			std::iter_swap(iter, std::prev(m_objectRegistry.end()));
			m_objectRegistry.erase(std::prev(m_objectRegistry.end()));
		}
		else
		{
			m_objectRegistry.erase(iter);
		}

		gameObject = GameObjectPtr();
	}
}

void GameScene::ProcessObjectRemoveDeferrals()
{
	// @TODO This is really ugly and slow, should go away with better object accessing/storage
	for (auto& object : m_objectsToRemove)
	{
		InternalRemoveGameObject(object);
	}
	m_objectsToRemove.clear();
}

void GameScene::RemoveGameObject(GameObjectPtr& gameObject)
{
	AssertNotNull(gameObject);
	gameObject->OnRemoveFromScene();
	m_objectsToRemove.emplace_back(gameObject);
}

GameObjectPtr GameScene::FindGameObjectByName(const std::string& name)
{
	// @TODO Slow first-pass quick implementation
	auto iter = std::find_if(m_objectRegistry.begin(), m_objectRegistry.end(),
		[&name](const std::unique_ptr<GameObject>& object)
		{
			return object->IsInScene() && object->GetName() == name;
		});

	if (iter != m_objectRegistry.end())
	{
		return GameObjectPtr((*iter).get());
	}

	return nullptr;
}

GameObjectPtr GameScene::FindGameObjectByGUID(ObjectGUID guid)
{
	const auto it = m_objectsByGUID.find(guid);
	if (it == m_objectsByGUID.end() || !it->second->IsInScene())
	{
		return GameObjectPtr();
	}
	return GameObjectPtr(it->second);
}

GameObjectPtr GameScene::AddGameObject(const std::string& name)
{
	GameObjectPtr gameObject = CreateGameObject();
	
	gameObject->SetName(name);

	AddGameObject(gameObject);

	return gameObject;
}

void GameScene::ForEachGameObject(Functor<void, GameObjectPtr> func)
{
	for (auto& gameObject : m_objectRegistry)
	{
		if (gameObject->IsInScene())
		{
			func(GameObjectPtr(gameObject.get()));
		}
	}
}

GameObjectPtr GameScene::CreateGameObject()
{
	std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(GenerateFreeGUID());
	gameObject->AddComponent<TransformComponent>();
	gameObject->Initialize();

	return GameObjectPtr(gameObject.release());
}

Vector3D GameScene::CalculateSceneCenter() const
{
	bool hasRenderedObject = false;
	Vector3D boundsMin;
	Vector3D boundsMax;

	for (const auto& gameObject : m_objectRegistry)
	{
		if (!gameObject->IsInScene() || !gameObject->IsRoot() || gameObject->GetComponent<RenderComponent>() == nullptr)
		{
			continue;
		}

		const Vector3D& position = gameObject->GetTransformComponent()->GetPosition();
		if (!hasRenderedObject)
		{
			boundsMin = position;
			boundsMax = position;
			hasRenderedObject = true;
			continue;
		}

		boundsMin = Vector3D(std::min(boundsMin.X(), position.X()), std::min(boundsMin.Y(), position.Y()), std::min(boundsMin.Z(), position.Z()));
		boundsMax = Vector3D(std::max(boundsMax.X(), position.X()), std::max(boundsMax.Y(), position.Y()), std::max(boundsMax.Z(), position.Z()));
	}

	return (boundsMin + boundsMax) * 0.5f;
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	OPTICK_EVENT();
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		// @todo eventually we should _only_ store root objects maybe?
		// feels like there could be some gross maintaining that during
		// AddChild/RemoveChild calls...
		if (gameObject->IsRoot())
		{
			gameObject->Update();
		}
	}
}

void GameScene::ShutDown()
{
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		gameObject->Uninitialize();
	}
	m_objectRegistry.clear();
	m_objectsByGUID.clear();
}