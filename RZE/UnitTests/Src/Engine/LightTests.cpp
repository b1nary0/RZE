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

TEST_CASE("Only changes that move a light's shadows change the scene's revision")
{
	RenderScene scene;
	LightObjectPtr light = scene.CreateLightObject(ELightType::Point);

	U64 revision = scene.GetRevision();
	light->SetColour(Vector4D(0.5f));
	light->SetStrength(4.0f);
	// A point light shines every way, so its direction changes nothing
	light->SetDirection(Vector3D(1.0f, 0.0f, 0.0f));
	CHECK(scene.GetRevision() == revision);

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
