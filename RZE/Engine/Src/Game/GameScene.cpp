#include <StdAfx.h>
#include <Game/GameScene.h>

#include <RapidJSON/document.h>
#include <RapidJSON/writer.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <Utils/Math/Quaternion.h>

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize()
{
	//mEntityHandler.Initialize();

	//// #TODO(Josh::This is the order of update. Need to make it so we can call these whenever)
	//mEntityHandler.AddSystem<LifetimeSystem>();
	//mEntityHandler.AddSystem<RenderSystem>();
}

void GameScene::NewScene()
{
	Load(FilePath("Assets/Scenes/Default.scene"));
}

void GameScene::Save(FilePath filePath)
{
	rapidjson::StringBuffer buf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

	//writer.StartObject();
	//{
	//	writer.String("entities");
	//	writer.StartObject();
	//	{
	//		for (const SceneEntryTemp& entry : mEntityEntries)
	//		{

	//			Apollo::EntityHandler::ComponentNameIDMap compMap;
	//			mEntityHandler.GetComponentNames(entry.ID, compMap);

	//			// We know everything has a name component, so just save the only thing they should have...
	//			// A name.
	//			const NameComponent* const nameComponent = mEntityHandler.GetComponent<NameComponent>(entry.ID);
	//			AssertNotNull(nameComponent);

	//			writer.String(nameComponent->Name.c_str());
	//			writer.StartObject();
	//			{
	//				writer.String("components");
	//				writer.StartObject();
	//				{
	//					for (auto& dataPair : compMap)
	//					{
	//						Apollo::ComponentBase* component = mEntityHandler.GetComponentByID(entry.ID, dataPair.first);
	//						component->Save(writer);
	//					}
	//				}
	//				writer.EndObject();
	//			}
	//			writer.EndObject();
	//		}
	//	}

	//	writer.EndObject();
	//}
	//writer.EndObject();
	
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
	Clear();

	mCurrentScenePath = filePath;

	File sceneFile(mCurrentScenePath);
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();

	rapidjson::Document sceneDoc;
	sceneDoc.Parse(sceneFile.Content().c_str());

	//rapidjson::Value::MemberIterator root = sceneDoc.FindMember("entities");
	//if (root != sceneDoc.MemberEnd())
	//{
	//	//
	//	// Entity
	//	//
	//	rapidjson::Value& rootVal = root->value;
	//	for (auto& entity = rootVal.MemberBegin(); entity != rootVal.MemberEnd(); ++entity)
	//	{
	//		Apollo::EntityID id = CreateEntity(entity->name.GetString());

	//		//
	//		// Components
	//		//
	//		const Apollo::EntityHandler::ComponentNameIDMap& componentInfo = mEntityHandler.GetAllComponentTypes();
	//		// ComponentBegin
	//		rapidjson::Value& val = entity->value;
	//		for (auto& member = val.MemberBegin(); member != val.MemberEnd(); ++member)
	//		{
	//			rapidjson::Value& compVal = member->value;
	//			for (auto& dataPair : componentInfo)
	//			{
	//				rapidjson::Value::MemberIterator compData = compVal.FindMember(dataPair.second.c_str());
	//				if (compData != compVal.MemberEnd())
	//				{
	//					Apollo::ComponentBase* component = mEntityHandler.AddComponentByID(id, dataPair.first);
	//					component->Load(compData->value);
	//				}
	//			}
	//		}
	//	}
	//}
}

void GameScene::Unload()
{
	Clear();
}

//Apollo::EntityID GameScene::CreateEntity(const std::string& name)
//{
//	Apollo::EntityID newEnt = mEntityHandler.CreateEntity(name);
//	mEntityHandler.AddComponent<NameComponent>(newEnt, name);
//	mEntityHandler.AddComponent<TransformComponent>(newEnt);
//
//	AddToScene(newEnt, name);
//
//	return newEnt;
//}

//void GameScene::DestroyEntity(Apollo::EntityID entity)
//{
//	mEntityHandler.DestroyEntity(entity);
//
//	auto it = std::remove_if(mEntityEntries.begin(), mEntityEntries.end(), [&entity](SceneEntryTemp& entry)
//	{
//		return entry.ID == entity;
//	});
//
//	if (it != mEntityEntries.end())
//	{
//		mEntityEntries.erase(it);
//	}
//}

//void GameScene::AddToScene(Apollo::EntityID entityID, const std::string& name)
//{
//	mEntityEntries.emplace_back();
//	mEntityEntries.back().ID = entityID;
//	mEntityEntries.back().Name = name;
//}

void GameScene::Clear()
{
	// #TODO(Josh::Remember to take a look at what SceneEntryTemp was needed for. Basically was a hack
	//				to quickly let me represent the scene in data.
	for (SceneEntryTemp sceneEntry : mEntityEntries)
	{
		// #TODO(Josh::Also probably have a function on EntityHandler that will clear it's data of the scene.
		//mEntityHandler.DestroyEntity(sceneEntry.ID);
	}
	mEntityEntries.clear();
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	OPTICK_EVENT();

	//mEntityHandler.Update();
}

void GameScene::ShutDown()
{
	//mEntityHandler.ShutDown();
}