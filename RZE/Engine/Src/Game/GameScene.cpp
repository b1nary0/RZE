#include <StdAfx.h>
#include <Game/GameScene.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Systems/LifetimeSystem.h>
#include <ECS/Systems/RenderSystem.h>

#include <RapidJSON/document.h>
#include <RapidJSON/writer.h>
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
	mEntityHandler.Initialize();

	// #TODO(Josh::This is the order of update. Need to make it so we can call these whenever)
	mEntityHandler.AddSystem<LifetimeSystem>();
	mEntityHandler.AddSystem<RenderSystem>();
}

void GameScene::NewScene()
{
	Load(FilePath("Assets/Scenes/Default.scene"));
}

void GameScene::Load(FilePath filePath)
{
	// #TODO(Josh) This whole function.
	// * Need to properly account for any additions to the engine/game component library here. *
	// * Need to be able to read from the .scene and determine which components need to be added that could be from			*
	// * either engine or game registration.																				*
	// * One possible solution is to generate an id and save to a registry on disk and read that and create from that id	*
	// * Could have issues with conflict resolution (file no longer shares in-memory id to compare against)					*
	// * This problem is the largest one so far for scene creation from a file.												*
	// - Load into memory via scene representation
	// - Probably need to generate GUIDS for components on each machine (installation)
	//		- Serialize these GUIDs as identifiers?
	//
	Clear();

	mCurrentScenePath = filePath;

	File sceneFile(mCurrentScenePath.GetAbsolutePath());
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();

	rapidjson::Document sceneDoc;
	sceneDoc.Parse(sceneFile.Content().c_str());

	rapidjson::Value::MemberIterator root = sceneDoc.FindMember("entities");
	if (root != sceneDoc.MemberEnd())
	{
		rapidjson::Value& rootVal = root->value;
		for (auto& entity = rootVal.MemberBegin(); entity != rootVal.MemberEnd(); ++entity)
		{
			Apollo::EntityID id = CreateEntity(entity->name.GetString());

			rapidjson::Value& val = entity->value;
			for (auto& member = val.MemberBegin(); member != val.MemberEnd(); ++member)
			{
				rapidjson::Value& comVal = member->value;
				rapidjson::Value::MemberIterator comp = comVal.FindMember("TransformComponent");
				if (comp != comVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;
					Vector3D position(memVal["Position"][0].GetFloat(), memVal["Position"][1].GetFloat(), memVal["Position"][2].GetFloat());
					Vector3D scale(memVal["Scale"][0].GetFloat(), memVal["Scale"][1].GetFloat(), memVal["Scale"][2].GetFloat());
					Vector3D rotation(memVal["Rotation"][0].GetFloat(), memVal["Rotation"][1].GetFloat(), memVal["Rotation"][2].GetFloat());

					GetEntityHandler().AddComponent<TransformComponent>(id, position, rotation, scale);
				}

				comp = comVal.FindMember("MeshComponent");
				if (comp != comVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;
					GetEntityHandler().AddComponent<MeshComponent>(id, FilePath(memVal["ResourcePath"].GetString()));
				}

				comp = comVal.FindMember("LightSourceComponent");
				if (comp != comVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;

					ELightType lightType = static_cast<ELightType>(memVal["LightType"].GetUint());
					Vector3D color(memVal["Color"][0].GetFloat(), memVal["Color"][1].GetFloat(), memVal["Color"][2].GetFloat());
					float strength = memVal["Strength"].GetFloat();

					GetEntityHandler().AddComponent<LightSourceComponent>(id, lightType, color, strength);
				}

				comp = comVal.FindMember("CameraComponent");
				if (comp != comVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;
					
					float fov = memVal["FOV"].GetFloat(); 
					float nearCull = memVal["NearCull"].GetFloat();
					float farCull = memVal["FarCull"].GetFloat();
					Vector3D forward(memVal["Forward"][0].GetFloat(), memVal["Forward"][1].GetFloat(), memVal["Forward"][2].GetFloat());
					Vector3D upDir(memVal["UpDir"][0].GetFloat(), memVal["UpDir"][1].GetFloat(), memVal["UpDir"][2].GetFloat());

					CameraComponent* const camComp = GetEntityHandler().AddComponent<CameraComponent>(id);
					camComp->FOV = fov;
					camComp->NearCull = nearCull;
					camComp->FarCull = farCull;
					camComp->Forward = forward;
					camComp->UpDir = upDir;
				}
			}
		}
	}
}

Apollo::EntityID GameScene::CreateEntity(const std::string& name)
{
	Apollo::EntityID newEnt = mEntityHandler.CreateEntity(name);
	mEntityHandler.AddComponent<NameComponent>(newEnt, name);
	mEntityHandler.AddComponent<TransformComponent>(newEnt);

	AddToScene(newEnt, name);

	return newEnt;
}

void GameScene::DestroyEntity(Apollo::EntityID entity)
{
	mEntityHandler.DestroyEntity(entity);

	auto it = std::remove_if(mEntityEntries.begin(), mEntityEntries.end(), [&entity](SceneEntryTemp& entry)
	{
		return entry.ID == entity;
	});

	if (it != mEntityEntries.end())
	{
		mEntityEntries.erase(it);
	}
}

void GameScene::AddToScene(Apollo::EntityID entityID, const std::string& name)
{
	mEntityEntries.emplace_back();
	mEntityEntries.back().ID = entityID;
	mEntityEntries.back().Name = name;
}

void GameScene::Clear()
{
	// #TODO(Josh::Remember to take a look at what SceneEntryTemp was needed for. Basically was a hack
	//				to quickly let me represent the scene in data.
	for (SceneEntryTemp sceneEntry : mEntityEntries)
	{
		// #TODO(Josh::Also probably have a function on EntityHandler that will clear it's data of the scene.
		mEntityHandler.DestroyEntity(sceneEntry.ID);
	}
	mEntityEntries.clear();
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	OPTICK_EVENT();

	mEntityHandler.Update();
}

void GameScene::Finish()
{
	mEntityHandler.ShutDown();
}