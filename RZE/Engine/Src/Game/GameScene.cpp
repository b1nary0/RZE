#include <StdAfx.h>
#include <Game/GameScene.h>

#include <Game/World/GameObject/GameObject.h>

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
		for (auto& entity = rootVal.MemberBegin(); entity != rootVal.MemberEnd(); ++entity)
		{
			rapidjson::Value& val = entity->value;
			
			GameObject* gameObject = new GameObject(entity->name.GetString());
			// ComponentBegin
			gameObject->Load(val);
			AddGameObject(*gameObject);
		}
	}
}

void GameScene::Unload()
{
}

void GameScene::AddGameObject(GameObject& gameObject)
{
	// #TODO Slow function

	const auto it = std::find(m_objectRegistry.begin(), m_objectRegistry.end(), &gameObject);
	AssertMsg(it == m_objectRegistry.end(), "GameObject already exists in scene");
	if (it == m_objectRegistry.end())
	{
		m_objectRegistry.push_back(&gameObject);
		// #TODO AddToScene stuff
		gameObject.OnAddToScene();
	}
}

void GameScene::RemoveGameObject(GameObject& gameObject)
{
	// #TODO Slow function

	const auto it = std::find(m_objectRegistry.begin(), m_objectRegistry.end(), &gameObject);
	AssertMsg(it != m_objectRegistry.end(), "GameObject doesn't exist in scene");
	if (it != m_objectRegistry.end())
	{
		m_objectRegistry.erase(it);
	// #TODO RemoveFromScene stuff
	}
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