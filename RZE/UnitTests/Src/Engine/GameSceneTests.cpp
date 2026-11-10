#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/LightComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderScene.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/File.h>

#include <TestFolder.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

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

TEST_CASE_FIXTURE(TwoScenes, "An object's components load and save in the scene file's order, on every platform")
{
	const char* const transform = R"("TransformComponent": {
		"Position": [0.0, 0.0, 0.0], "Scale": [1.0, 1.0, 1.0], "Rotation": [0.0, 0.0, 0.0] })";
	const char* const light = R"("LightComponent": {
		"Type": "Point", "Range": 10.0, "LightStrength": 1.0, "LightColour": [1.0, 1.0, 1.0] })";

	// Both ways round, so neither can pass by matching the registry's own order
	for (const bool isLightFirst : { true, false })
	{
		CAPTURE(isLightFirst);
		const std::string json = std::string(R"({ "name": "Ordered", "components": { )") +
			(isLightFirst ? light : transform) + ", " + (isLightFirst ? transform : light) + " } }";
		rapidjson::Document entry;
		entry.Parse(json.c_str());
		REQUIRE_FALSE(entry.HasParseError());

		GameObjectPtr loaded = First.DeserializeGameObject(entry, nullptr);
		const std::vector<GameObjectComponentID> expected = isLightFirst
			? std::vector<GameObjectComponentID>{ LightComponent::StaticTypeID(), TransformComponent::StaticTypeID() }
			: std::vector<GameObjectComponentID>{ TransformComponent::StaticTypeID(), LightComponent::StaticTypeID() };
		std::vector<GameObjectComponentID> order;
		for (const GameObjectComponentBase* component : loaded->GetComponents())
		{
			order.push_back(component->GetTypeID());
		}
		CHECK(order == expected);

		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		loaded->Save(writer);
		rapidjson::Document saved;
		saved.Parse(buffer.GetString());
		REQUIRE_FALSE(saved.HasParseError());
		const rapidjson::Value& components = saved["components"];
		REQUIRE(components.MemberCount() == 2);
		CHECK(std::string(components.MemberBegin()->name.GetString()) == (isLightFirst ? "LightComponent" : "TransformComponent"));
	}
}

namespace
{
	std::vector<std::string> g_warnings;

	void CollectWarnings(Debug::ELogLevel level, std::string_view line)
	{
		if (level == Debug::ELogLevel::Warning)
		{
			g_warnings.emplace_back(line);
		}
	}
}

TEST_CASE_FIXTURE(TwoScenes, "A component this build doesn't have is skipped with a warning, and the rest still loads")
{
	rapidjson::Document entry;
	entry.Parse(R"({ "name": "Renamed", "components": {
		"NoSuchComponent": { "Speed": 1.0 },
		"TransformComponent": { "Position": [1.0, 2.0, 3.0], "Scale": [1.0, 1.0, 1.0], "Rotation": [0.0, 0.0, 0.0] } } })");
	REQUIRE_FALSE(entry.HasParseError());

	g_warnings.clear();
	Debug::SetLogSink(&CollectWarnings);
	GameObjectPtr loaded = First.DeserializeGameObject(entry, nullptr);
	Debug::SetLogSink(nullptr);

	REQUIRE(loaded->GetComponents().size() == 1);
	CHECK(loaded->GetComponents()[0]->GetTypeID() == TransformComponent::StaticTypeID());
	CHECK(loaded->GetTransformComponent()->GetPosition() == Vector3D(1.0f, 2.0f, 3.0f));

#ifdef RZE_LOGGING_ENABLED
	REQUIRE(g_warnings.size() == 1);
	CHECK(g_warnings[0].find("[Renamed]") != std::string::npos);
	CHECK(g_warnings[0].find("[NoSuchComponent]") != std::string::npos);
#else
	CHECK(g_warnings.empty());
#endif
}

TEST_CASE("A component is named as its class is, which the Inspector shows")
{
	CHECK(std::string(LightComponent::GetComponentName()) == "LightComponent");
	CHECK(std::string(TransformComponent::GetComponentName()) == "TransformComponent");
}

TEST_CASE("Serialize makes a path the scene's file only once it's written there, and says when it couldn't")
{
	const ScopedTestFolder folder("UnitTestsTemp_GameSceneSave");
	GameScene scene;
	scene.AddGameObject("Saved");

	const Filepath first = folder.GetFile("First.scene");
	REQUIRE(scene.Serialize(first));
	CHECK(first.Exists());
	CHECK(scene.GetCurrentScenePath() == first);
	CHECK_FALSE(File::GetTemporaryPathFor(first).Exists());
	// An invalid path saves to the current file
	CHECK(scene.Serialize(Filepath()));

	SUBCASE("Its temporary file can't be opened")
	{
		const Filepath blocked = folder.GetFile("Blocked.scene");
		folder.AddFolder("Blocked.scene.tmp");
		CHECK_FALSE(scene.Serialize(blocked));
		CHECK_FALSE(blocked.Exists());
	}

	SUBCASE("It can't be moved over the file")
	{
		const Filepath blocked = folder.GetFile("Blocked.scene");
		folder.AddFolder("Blocked.scene");
		CHECK_FALSE(scene.Serialize(blocked));
		CHECK(std::filesystem::is_directory(blocked.GetAbsolutePath()));
		CHECK_FALSE(File::GetTemporaryPathFor(blocked).Exists());
	}

	CHECK(scene.GetCurrentScenePath() == first);
	scene.Unload();
}
