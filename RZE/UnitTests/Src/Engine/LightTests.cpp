#include <doctest/doctest.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/LightComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/RenderScene.h>
#include <Graphics/SceneChangeLog.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

namespace
{
	struct LightScene
	{
		GameScene Scene;
		~LightScene() { Scene.Unload(); }

		// As a scene file has it, with the light's own data after the transform
		GameObjectPtr Load(const char* lightComponent)
		{
			std::string json = R"({ "name": "Light", "components": { "TransformComponent": {
				"Position": [0.0, 0.0, 0.0], "Scale": [1.0, 1.0, 1.0], "Rotation": [0.0, 0.0, 0.0] }, )";
			json += lightComponent;
			json += " } }";

			rapidjson::Document entry;
			entry.Parse(json.c_str());
			REQUIRE_FALSE(entry.HasParseError());

			return Scene.DeserializeGameObject(entry, nullptr);
		}
	};

	// What Serialize writes, read back as JSON
	rapidjson::Document Save(LightComponent& light)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		writer.StartObject();
		light.Serialize(writer);
		writer.EndObject();

		rapidjson::Document document;
		document.Parse(buffer.GetString());
		return document;
	}
}

TEST_CASE("A new light component is a point light")
{
	const LightComponent light;
	CHECK(light.GetLightType() == ELightType::Point);
}

TEST_CASE("A light component keeps its type, range, strength and colour through saving and loading")
{
	LightComponent saved;
	saved.SetLightType(ELightType::Directional);
	saved.SetRange(25.0f);
	saved.SetLightStrength(3.5f);
	saved.SetLightColour(Vector4D(0.25f, 0.5f, 0.75f, 1.0f));

	const rapidjson::Document document = Save(saved);
	REQUIRE(document.HasMember("LightComponent"));
	CHECK(std::string(document["LightComponent"]["Type"].GetString()) == "Directional");

	LightComponent loaded;
	loaded.Deserialize(document["LightComponent"]);
	CHECK(loaded.GetLightType() == ELightType::Directional);
	CHECK(loaded.GetRange() == doctest::Approx(25.0f));
	CHECK(loaded.GetLightStrength() == doctest::Approx(3.5f));
	CHECK(loaded.GetLightColour().X() == doctest::Approx(0.25f));
	CHECK(loaded.GetLightColour().Y() == doctest::Approx(0.5f));
	CHECK(loaded.GetLightColour().Z() == doctest::Approx(0.75f));
}

TEST_CASE_FIXTURE(LightScene, "A light saved without a type loads as a point light, like a new one")
{
	GameObjectPtr object = Load(R"("LightComponent": { "LightStrength": 0.5, "LightColour": [1.0, 1.0, 1.0] })");

	GameObjectComponentPtr<LightComponent> light = object->GetComponent<LightComponent>();
	REQUIRE(light != nullptr);
	CHECK(light->GetLightType() == ELightType::Point);
	CHECK(light->GetLightStrength() == doctest::Approx(0.5f));
}

TEST_CASE_FIXTURE(LightScene, "A light in a scene puts a light of its type in the render scene")
{
	GameObjectPtr object = Load(R"("LightComponent": { "Type": "Point", "Range": 4.0, "LightStrength": 2.0, "LightColour": [1.0, 1.0, 1.0] })");
	object->GetTransformComponent()->SetPosition(Vector3D(1.0f, 2.0f, 3.0f));
	object->GetComponent<LightComponent>()->Update();

	const RenderScene& renderScene = Scene.GetRenderScene();
	REQUIRE(renderScene.GetLightObjects().size() == 1);
	const LightObject& lightObject = *renderScene.GetLightObjects()[0];
	CHECK(lightObject.GetType() == ELightType::Point);
	CHECK(lightObject.GetRange() == doctest::Approx(4.0f));
	CHECK(lightObject.GetPosition() == Vector3D(1.0f, 2.0f, 3.0f));
	CHECK(renderScene.GetDirectionalLight() == nullptr);
}

TEST_CASE("The scene's directional light is its first directional one, whatever comes before it")
{
	RenderScene scene;
	CHECK(scene.GetDirectionalLight() == nullptr);

	LightObjectPtr point = scene.CreateLightObject(ELightType::Point);
	CHECK(scene.GetDirectionalLight() == nullptr);

	LightObjectPtr sun = scene.CreateLightObject(ELightType::Directional);
	LightObjectPtr secondSun = scene.CreateLightObject(ELightType::Directional);
	REQUIRE(scene.GetDirectionalLight() != nullptr);
	CHECK(scene.GetDirectionalLight()->GetLiveID() == sun->GetLiveID());

	scene.DestroyLightObject(sun);
	REQUIRE(scene.GetDirectionalLight() != nullptr);
	CHECK(scene.GetDirectionalLight()->GetLiveID() == secondSun->GetLiveID());

	scene.DestroyLightObject(point);
	scene.DestroyLightObject(secondSun);
}

TEST_CASE("A point light falls off from its full strength at the light to nothing at its range")
{
	CHECK(LightObject::CalculatePointFalloff(0.0f) == doctest::Approx(1.0f));
	CHECK(LightObject::CalculatePointFalloff(1.0f) == doctest::Approx(0.0f));
	CHECK(LightObject::CalculatePointFalloff(1.5f) == doctest::Approx(0.0f));
	CHECK(LightObject::CalculatePointFalloff(0.25f) > LightObject::CalculatePointFalloff(0.5f));
}

TEST_CASE("A stronger point light outshines the ambient light further out, but never past its range")
{
	const Vector4D white(1.0f);
	const float dim = LightObject::CalculateOutshinesAmbientRatio(0.5f, white);
	const float normal = LightObject::CalculateOutshinesAmbientRatio(5.0f, white);
	const float bright = LightObject::CalculateOutshinesAmbientRatio(500.0f, white);

	CHECK(dim > 0.0f);
	CHECK(dim < normal);
	CHECK(normal < bright);
	CHECK(bright < 1.0f);

	// Twice as strong is exactly where the original falls off to half
	CHECK(LightObject::CalculatePointFalloff(LightObject::CalculateOutshinesAmbientRatio(10.0f, white))
		== doctest::Approx(LightObject::CalculatePointFalloff(normal) * 0.5f).epsilon(0.01));

	// Fainter than the ambient light even right next to it
	CHECK(LightObject::CalculateOutshinesAmbientRatio(0.01f, white) == 0.0f);
	CHECK(LightObject::CalculateOutshinesAmbientRatio(5.0f, Vector4D(0.0f, 0.0f, 0.0f, 1.0f)) == 0.0f);
}

TEST_CASE("A wire sphere, as drawn for a point light's range, is closed circles on the sphere")
{
	RenderScene scene;
	const Vector3D centre(1.0f, 2.0f, 3.0f);
	scene.DrawWireSphere(centre, 4.0f, Vector3D(1.0f));

	const DebugLineContainer& lines = scene.GetDebugLines();
	REQUIRE_FALSE(lines.empty());
	CHECK(lines.size() % 3 == 0);
	for (const DebugLine& line : lines)
	{
		CHECK((line.start - centre).Length() == doctest::Approx(4.0f).epsilon(0.001));
		CHECK((line.end - centre).Length() == doctest::Approx(4.0f).epsilon(0.001));
	}

	// Each circle ends where it started
	const size_t segmentsPerCircle = lines.size() / 3;
	for (size_t circle = 0; circle < 3; ++circle)
	{
		const DebugLine& first = lines[circle * segmentsPerCircle];
		const DebugLine& last = lines[(circle + 1) * segmentsPerCircle - 1];
		CHECK((last.end - first.start).Length() == doctest::Approx(0.0f).epsilon(0.001));
	}

	scene.ClearDebugLines();
}

TEST_CASE("Only changes to how a light lights the scene change the scene's revision")
{
	RenderScene scene;
	LightObjectPtr light = scene.CreateLightObject(ELightType::Point);

	// A point light shines every way, so its direction changes nothing, and nor does setting what it already has
	U64 revision = scene.GetRevision();
	light->SetDirection(Vector3D(1.0f, 0.0f, 0.0f));
	light->SetColour(light->GetColour());
	light->SetStrength(light->GetStrength());
	CHECK(scene.GetRevision() == revision);

	light->SetColour(Vector4D(0.5f));
	CHECK(scene.GetRevision() != revision);

	revision = scene.GetRevision();
	light->SetStrength(4.0f);
	CHECK(scene.GetRevision() != revision);

	revision = scene.GetRevision();
	light->SetPosition(Vector3D(1.0f, 0.0f, 0.0f));
	CHECK(scene.GetRevision() != revision);

	revision = scene.GetRevision();
	light->SetRange(20.0f);
	CHECK(scene.GetRevision() != revision);

	revision = scene.GetRevision();
	light->SetType(ELightType::Directional);
	CHECK(scene.GetRevision() != revision);

	// A directional light is the same everywhere, so its position changes nothing, but its direction does
	revision = scene.GetRevision();
	light->SetPosition(Vector3D(5.0f, 0.0f, 0.0f));
	light->SetRange(30.0f);
	CHECK(scene.GetRevision() == revision);

	light->SetDirection(Vector3D(0.0f, 0.0f, -1.0f));
	CHECK(scene.GetRevision() != revision);

	scene.DestroyLightObject(light);
}

TEST_CASE("A light's lighting revision is the scene revision when it was made or last changed how it lights")
{
	RenderScene scene;
	LightObjectPtr light = scene.CreateLightObject(ELightType::Point);
	CHECK(light->GetLightingRevision() == scene.GetRevision());
	CHECK(light->GetShadowRevision() == scene.GetRevision());

	// Colour and strength change the light but not its shadows
	light->SetStrength(3.0f);
	CHECK(light->GetLightingRevision() == scene.GetRevision());
	CHECK(light->GetShadowRevision() < light->GetLightingRevision());

	// Moving changes both, at the same revision
	light->SetPosition(Vector3D(1.0f, 0.0f, 0.0f));
	CHECK(light->GetLightingRevision() == scene.GetRevision());
	CHECK(light->GetShadowRevision() == light->GetLightingRevision());

	scene.DestroyLightObject(light);
}

TEST_CASE("A point light logs where it reached and where it reaches when it changes")
{
	RenderScene scene;
	LightObjectPtr light = scene.CreateLightObject(ELightType::Point);
	light->SetRange(2.0f);

	const SceneChangeLog& changes = scene.GetLightChanges();
	const BoundingBox atOrigin(Vector3D(-0.5f), Vector3D(0.5f));
	const BoundingBox atTen(Vector3D(9.5f, -0.5f, -0.5f), Vector3D(10.5f, 0.5f, 0.5f));
	const BoundingBox between(Vector3D(4.5f, -0.5f, -0.5f), Vector3D(5.5f, 0.5f, 0.5f));

	U64 revision = scene.GetRevision();
	CHECK(light->GetReach() == BoundingBox(Vector3D(-2.0f), Vector3D(2.0f)));
	light->SetPosition(Vector3D(10.0f, 0.0f, 0.0f));
	CHECK(changes.HasChangedWithin(atOrigin, revision));
	CHECK(changes.HasChangedWithin(atTen, revision));
	// Logged apart, so what's between isn't marked
	CHECK_FALSE(changes.HasChangedWithin(between, revision));

	// A brighter light lights the same place differently
	revision = scene.GetRevision();
	light->SetStrength(8.0f);
	CHECK(changes.HasChangedWithin(atTen, revision));
	CHECK_FALSE(changes.HasChangedWithin(atOrigin, revision));

	// Destroyed, it no longer lights where it reached
	revision = scene.GetRevision();
	scene.DestroyLightObject(light);
	CHECK(changes.HasChangedWithin(atTen, revision));
	CHECK_FALSE(changes.HasChangedWithin(atOrigin, revision));
}

TEST_CASE("A directional light changes the light everywhere")
{
	RenderScene scene;
	LightObjectPtr sun = scene.CreateLightObject(ELightType::Directional);
	CHECK(sun->GetReach().IsEmpty());

	const SceneChangeLog& changes = scene.GetLightChanges();
	const BoundingBox farAway(Vector3D(1000.0f), Vector3D(1001.0f));

	const U64 revision = scene.GetRevision();
	CHECK_FALSE(changes.HasChangedWithin(farAway, revision));

	sun->SetColour(Vector4D(1.0f, 0.9f, 0.8f, 1.0f));
	CHECK(changes.HasChangedWithin(farAway, revision));
	CHECK_FALSE(changes.ForEachChangeSince(revision, [](const BoundingBox&) {}));

	// Becoming a point light changes it everywhere too, as it no longer lights what's beyond its range
	const U64 pointRevision = scene.GetRevision();
	sun->SetType(ELightType::Point);
	CHECK(changes.HasChangedWithin(farAway, pointRevision));

	scene.DestroyLightObject(sun);
}

TEST_CASE("A light's shadow revision is the scene revision when it was made or its shadows last moved")
{
	RenderScene scene;
	LightObjectPtr first = scene.CreateLightObject(ELightType::Point);
	CHECK(first->GetShadowRevision() == scene.GetRevision());

	LightObjectPtr second = scene.CreateLightObject(ELightType::Point);
	const U64 secondRevision = second->GetShadowRevision();
	CHECK(secondRevision == scene.GetRevision());

	first->SetPosition(Vector3D(2.0f, 0.0f, 0.0f));
	CHECK(first->GetShadowRevision() == scene.GetRevision());
	// Only the light that moved
	CHECK(second->GetShadowRevision() == secondRevision);

	// Colour and strength light differently but cast the same shadows
	const U64 firstRevision = first->GetShadowRevision();
	first->SetColour(Vector4D(0.2f));
	first->SetStrength(9.0f);
	CHECK(first->GetShadowRevision() == firstRevision);

	first->SetType(ELightType::Directional);
	CHECK(first->GetShadowRevision() > firstRevision);
	CHECK(first->GetShadowRevision() == scene.GetRevision());

	scene.DestroyLightObject(first);
	scene.DestroyLightObject(second);
}

TEST_CASE("A light in no scene has no shadow revision")
{
	LightObject light(ELightType::Point);
	light.SetPosition(Vector3D(1.0f, 0.0f, 0.0f));
	CHECK(light.GetShadowRevision() == 0);
}
