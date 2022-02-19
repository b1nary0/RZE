#include <StdAfx.h>
#include <Game/GameScene.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <RapidJSON/document.h>
#include <RapidJSON/writer.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Quaternion.h>

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
	Load(FilePath("Assets/Scenes/Default.scene"));
}

void GameScene::Save(FilePath filePath)
{
	rapidjson::StringBuffer buf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

	writer.StartObject();
	{
		writer.String("gameobjects");
		writer.StartObject();
		{
			for (auto& gameObject : m_objectRegistry)
			{
				gameObject->Save(writer);
			}
		}

		writer.EndObject();
	}
	writer.EndObject();
	
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

void GameScene::Load(FilePath filePath)
{
	mCurrentScenePath = filePath;

	File sceneFile(mCurrentScenePath);
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();

	rapidjson::Document sceneDoc;
	sceneDoc.Parse(sceneFile.Content().c_str());

	rapidjson::Value::MemberIterator root = sceneDoc.FindMember("entities");
	if (root != sceneDoc.MemberEnd())
	{
		//
		// Entity
		//
		rapidjson::Value& rootVal = root->value;
		for (auto& object = rootVal.MemberBegin(); object != rootVal.MemberEnd(); ++object)
		{
			rapidjson::Value& val = object->value;
			
			std::shared_ptr<GameObject> gameObject = CreateGameObjectNoComponents();
			gameObject->SetName(object->name.GetString());
			// ComponentBegin
			gameObject->Load(val);
			AddGameObject(gameObject);
		}
	}
}

std::shared_ptr<GameObject> GameScene::CreateGameObjectNoComponents()
{
	return std::make_shared<GameObject>();
}

void GameScene::Unload()
{
}

void GameScene::AddGameObject(const std::shared_ptr<GameObject>& gameObject)
{
	// #TODO Slow function

	const auto it = std::find(m_objectRegistry.begin(), m_objectRegistry.end(), gameObject);
	AssertMsg(it == m_objectRegistry.end(), "GameObject already exists in scene");
	if (it == m_objectRegistry.end())
	{
		m_objectRegistry.push_back(gameObject);
		// #TODO AddToScene stuff
		gameObject->OnAddToScene();
	}
}

void GameScene::RemoveGameObject(const std::shared_ptr<GameObject>& gameObject)
{
	// #TODO Slow function

	const auto iter = std::find(m_objectRegistry.begin(), m_objectRegistry.end(), gameObject);
	AssertMsg(iter != m_objectRegistry.end(), "GameObject doesn't exist in scene");
	if (iter != m_objectRegistry.end())
	{
		if (m_objectRegistry.size() == 1)
		{
			m_objectRegistry.erase(iter);
		}
		else
		{
			std::iter_swap(iter, std::prev(m_objectRegistry.end()));
		}

		gameObject->OnRemoveFromScene();

		m_objectRegistry.erase(iter);
	}
}

std::shared_ptr<GameObject> GameScene::FindGameObjectByName(const std::string& name)
{
	// @TODO Slow first-pass quick implementation
	auto iter = std::find_if(m_objectRegistry.begin(), m_objectRegistry.end(),
		[&name](const std::shared_ptr<GameObject> object)
		{
			return object->GetName() == name;
		});

	if (iter != m_objectRegistry.end())
	{
		return *iter;
	}

	return nullptr;
}

std::shared_ptr<GameObject> GameScene::AddGameObject(const std::string& name)
{
	std::shared_ptr<GameObject> gameObject = CreateGameObject();
	gameObject->SetName(name);
	AddGameObject(gameObject);

	return gameObject;
}

void GameScene::ForEachGameObject(Functor<void, std::shared_ptr<GameObject>> func)
{
	for (auto& gameObject : m_objectRegistry)
	{
		func(gameObject);
	}
}

std::shared_ptr<GameObject> GameScene::CreateGameObject()
{
	std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>();
	gameObject->AddComponent<TransformComponent>();
	return gameObject;
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	OPTICK_EVENT();
	for (auto& gameObject : m_objectRegistry)
	{
		gameObject->Update();
	}
}

void GameScene::ShutDown()
{
}