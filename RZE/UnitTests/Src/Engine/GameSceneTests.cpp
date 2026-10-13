#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>

#include <Graphics/RenderScene.h>

#include <RapidJSON/document.h>

namespace
{
	// Two scenes side by side, like the active scene and one loaded in the background
	struct TwoScenes
	{
		GameScene First;
		GameScene Second;

		~TwoScenes()
		{
			First.Unload();
			Second.Unload();
		}
	};
}

TEST_CASE_FIXTURE(TwoScenes, "Game objects belong to the scene that made them, however they were made")
{
	GameObjectPtr named = First.AddGameObject("Named");
	CHECK(&named->GetScene() == &First);

	GameObjectPtr created = Second.CreateGameObject();
	Second.AddGameObject(created);
	CHECK(&created->GetScene() == &Second);

	rapidjson::Document entry;
	// As a scene file has it: every object has a transform, which attaching to a parent needs
	entry.Parse(R"({ "name": "Loaded", "components": { "TransformComponent": {
		"Position": [0.0, 0.0, 0.0], "Scale": [1.0, 1.0, 1.0], "Rotation": [0.0, 0.0, 0.0] } } })");
	REQUIRE_FALSE(entry.HasParseError());

	GameObjectPtr loaded = Second.DeserializeGameObject(entry, created);
	CHECK(&loaded->GetScene() == &Second);
	CHECK(loaded->GetParent() == created);
}

TEST_CASE_FIXTURE(TwoScenes, "An object still belongs to its scene while it's out of it")
{
	GameObjectPtr object = First.AddGameObject("SetAside");
	object->OnRemoveFromScene();

	CHECK_FALSE(object->IsInScene());
	CHECK(&object->GetScene() == &First);

	object->OnAddToScene();
}

TEST_CASE_FIXTURE(TwoScenes, "Each scene draws into its own render scene")
{
	CHECK(&First.GetRenderScene() != &Second.GetRenderScene());

	First.GetRenderScene().DrawLine(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f));

	CHECK(First.GetRenderScene().GetDebugLines().size() == 1);
	CHECK(Second.GetRenderScene().GetDebugLines().empty());

	First.GetRenderScene().ClearDebugLines();
}
