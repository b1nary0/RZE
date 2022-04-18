#include <StdAfx.h>
#include <Game/GameScene.h>

#include <Game/World/GameObject/GameObject.h>
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
	Load(Filepath("Assets/Scenes/Default.scene"));
}

void GameScene::Save(Filepath filePath)
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
				if (gameObject->IncludeInSave())
				{
					gameObject->Save(writer);
				}
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

void GameScene::Load(Filepath filePath)
{
	mCurrentScenePath = filePath;

	File sceneFile(mCurrentScenePath);
	AssertExpr(sceneFile.IsValid());
	sceneFile.Read();
	sceneFile.Close();

	rapidjson::Document sceneDoc;
	sceneDoc.Parse(sceneFile.Content().c_str());

	rapidjson::Value::MemberIterator root = sceneDoc.FindMember("gameobjects");
	if (root != sceneDoc.MemberEnd())
	{
		//
		// Entity
		//
		rapidjson::Value& rootVal = root->value;
		for (auto object = rootVal.MemberBegin(); object != rootVal.MemberEnd(); ++object)
		{
			rapidjson::Value& val = object->value;
			
			std::unique_ptr<GameObject> gameObject = CreateGameObjectNoComponents();
			gameObject->SetName(object->name.GetString());
			// ComponentBegin
			gameObject->Load(val);
			AddGameObject(std::move(gameObject));
		}
	}
}

std::unique_ptr<GameObject> GameScene::CreateGameObjectNoComponents()
{
	return std::make_unique<GameObject>();
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
		gameObject->OnAddToScene();
		m_objectRegistry.emplace_back(std::move(gameObject));
	}
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

GameObjectPtr GameScene::AddGameObject(const std::string& name)
{
	std::unique_ptr<GameObject> gameObject = CreateGameObject();
	gameObject->SetName(name);
	gameObject->Initialize();

	GameObjectPtr ptr(gameObject.get());

	AddGameObject(std::move(gameObject));

	return ptr;
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

std::unique_ptr<GameObject> GameScene::CreateGameObject()
{
	std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>();
	gameObject->AddComponent<TransformComponent>();
	return gameObject;
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
		gameObject->Update();
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
}