#include <StdAfx.h>
#include <Game/GameScene.h>

#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
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

	mEntityHandler.AddSystem<RenderSystem>();

	// #TODO(Josh) Test
	Load(FilePath("Engine/Assets/Scenes/Test.scene"));
}

void GameScene::Load(FilePath filePath)
{
	File sceneFile(filePath.GetAbsolutePath());
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();

	rapidjson::Document sceneDoc;
	sceneDoc.Parse(sceneFile.Content().c_str());

	for (auto& entity = sceneDoc.MemberBegin(); entity != sceneDoc.MemberEnd(); ++entity)
	{
		Apollo::EntityID id = GetEntityHandler().CreateEntity();

		rapidjson::Value& val = entity->value;
		for (auto& member = val.MemberBegin(); member != val.MemberEnd(); ++member)
		{
			rapidjson::Value& comVal = member->value;
			for (auto& component = comVal.MemberBegin(); component != comVal.MemberEnd(); ++component)
			{
				rapidjson::Value& comValVal = component->value;
				rapidjson::Value::MemberIterator comp = comValVal.FindMember("TransformComponent");
				if (comp != comValVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;
					Vector3D position(memVal["Position"][0].GetFloat(), memVal["Position"][1].GetFloat(), memVal["Position"][2].GetFloat());
					Vector3D scale(memVal["Scale"][0].GetFloat(), memVal["Scale"][1].GetFloat(), memVal["Scale"][2].GetFloat());

					GetEntityHandler().AddComponent<TransformComponent>(id, position, Quaternion(), scale);
				}

				comp = comValVal.FindMember("MeshComponent");
				if (comp != comValVal.MemberEnd())
				{
					rapidjson::Value& memVal = comp->value;
					GetEntityHandler().AddComponent<MeshComponent>(id, FilePath(memVal["ResourcePath"].GetString()));
				}
			}
		}
	}
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	mEntityHandler.Update();
}

void GameScene::Finish()
{
	mEntityHandler.ShutDown();
}