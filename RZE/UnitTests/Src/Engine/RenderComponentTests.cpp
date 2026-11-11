#include <doctest/doctest.h>

#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <string>

namespace
{
	// What Serialize writes, read back as JSON
	rapidjson::Document Save(RenderComponent& component)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		writer.StartObject();
		component.Serialize(writer);
		writer.EndObject();

		rapidjson::Document document;
		document.Parse(buffer.GetString());
		return document;
	}
}

TEST_CASE("A render component keeps its mobility through saving and loading, and is Static without one")
{
	RenderComponent saved;
	CHECK(saved.GetMobility() == ERenderMobility::Static);
	saved.SetMobility(ERenderMobility::Movable);

	const rapidjson::Document document = Save(saved);
	REQUIRE(document.HasMember("RenderComponent"));
	CHECK(std::string(document["RenderComponent"]["Mobility"].GetString()) == "Movable");

	RenderComponent loaded;
	loaded.Deserialize(document["RenderComponent"]);
	CHECK(loaded.GetMobility() == ERenderMobility::Movable);

	// Scenes saved before mobility existed
	rapidjson::Document older;
	older.Parse(R"({ "ResourcePath": "" })");
	RenderComponent fromOlder;
	fromOlder.Deserialize(older);
	CHECK(fromOlder.GetMobility() == ERenderMobility::Static);
}
