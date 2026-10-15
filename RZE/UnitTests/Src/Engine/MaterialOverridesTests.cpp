#include <doctest/doctest.h>

#include <Game/World/GameObjectComponents/RenderComponent.h>

#include <Graphics/Material.h>
#include <Graphics/MaterialOverrides.h>
#include <Graphics/StaticMesh.h>
#include <Graphics/StaticMeshInstance.h>

#include <memory>
#include <string>
#include <vector>

// Meshes here have slots without materials (making one needs the renderer), so slots keep their overrides but have no
// material of their own to draw with

namespace
{
	using EBlendMode = MaterialValues::EBlendMode;

	// The material asset a test mesh's slot is made from
	Filepath GetMaterialPath(const std::string& name)
	{
		return Filepath("ProjectData/Material/Test/" + name + ".materialasset");
	}

	// An instance of a mesh with a slot for each name, made from the material asset GetMaterialPath names, and no
	// geometry
	std::shared_ptr<StaticMeshInstance> MakeInstance(const std::vector<std::string>& slotNames)
	{
		std::vector<StaticMesh::Slot> slots;
		for (const std::string& name : slotNames)
		{
			StaticMesh::Slot& slot = slots.emplace_back();
			slot.Name = name;
			slot.MaterialPath = GetMaterialPath(name);
		}
		return StaticMeshInstance::Create(StaticMesh::Create("Test", std::vector<StaticMesh::SubMesh>(), std::move(slots)));
	}

	MaterialOverrides WithOpacity(float opacity)
	{
		MaterialOverrides overrides;
		overrides.Opacity = opacity;
		return overrides;
	}

	// Through the JSON a scene file keeps
	MaterialOverrides RoundTrip(const MaterialOverrides& overrides)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		overrides.Serialize(writer);

		rapidjson::Document document;
		document.Parse(buffer.GetString());
		REQUIRE_FALSE(document.HasParseError());
		return MaterialOverrides::Deserialize(document);
	}
}

TEST_CASE("Overrides replace only the values they set")
{
	MaterialValues base;
	base.BlendMode = EBlendMode::Opaque;
	base.Opacity = 1.0f;
	base.Shininess = 16.0f;
	base.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE] = Filepath("Assets/Textures/Base.png");
	base.Textures[MaterialInstance::TEXTURE_SLOT_NORMAL] = Filepath("Assets/Textures/Normal.png");

	MaterialOverrides overrides;
	CHECK(overrides.IsEmpty());
	overrides.BlendMode = EBlendMode::Transparent;
	overrides.Opacity = 0.3f;
	overrides.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE] = Filepath("Assets/Textures/Other.png");
	// Overridden to nothing: emptied
	overrides.Textures[MaterialInstance::TEXTURE_SLOT_NORMAL] = Filepath();
	CHECK_FALSE(overrides.IsEmpty());

	const MaterialValues values = overrides.ApplyTo(base);
	CHECK(values.BlendMode == EBlendMode::Transparent);
	CHECK(values.Opacity == doctest::Approx(0.3f));
	CHECK(values.Shininess == doctest::Approx(16.0f));
	CHECK(values.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE].GetRelativePath() == Filepath("Assets/Textures/Other.png").GetRelativePath());
	CHECK_FALSE(values.Textures[MaterialInstance::TEXTURE_SLOT_NORMAL].IsValid());
	CHECK_FALSE(values.Textures[MaterialInstance::TEXTURE_SLOT_SPECULAR].IsValid());

	CHECK(MaterialOverrides().ApplyTo(base).Shininess == doctest::Approx(16.0f));
}

TEST_CASE("Overrides survive being saved and loaded, with only what's overridden written")
{
	MaterialOverrides overrides;
	overrides.BlendMode = EBlendMode::Additive;
	overrides.Shininess = 8.0f;
	overrides.Textures[MaterialInstance::TEXTURE_SLOT_OPACITY] = Filepath("Assets/Textures/Mask.png");
	overrides.Textures[MaterialInstance::TEXTURE_SLOT_HEIGHT] = Filepath();
	overrides.InvertOpacityMask = true;

	const MaterialOverrides loaded = RoundTrip(overrides);
	CHECK(loaded.BlendMode == EBlendMode::Additive);
	CHECK(loaded.Shininess == doctest::Approx(8.0f));
	CHECK_FALSE(loaded.Opacity.has_value());
	REQUIRE(loaded.Textures[MaterialInstance::TEXTURE_SLOT_OPACITY].has_value());
	CHECK(loaded.Textures[MaterialInstance::TEXTURE_SLOT_OPACITY]->GetRelativePath() == Filepath("Assets/Textures/Mask.png").GetRelativePath());
	// An emptied slot stays emptied, not unset
	REQUIRE(loaded.Textures[MaterialInstance::TEXTURE_SLOT_HEIGHT].has_value());
	CHECK_FALSE(loaded.Textures[MaterialInstance::TEXTURE_SLOT_HEIGHT]->IsValid());
	CHECK_FALSE(loaded.Textures[MaterialInstance::TEXTURE_SLOT_DIFFUSE].has_value());
	CHECK(loaded.InvertOpacityMask == true);
	CHECK_FALSE(loaded.OpacityMaskInAlpha.has_value());

	CHECK(RoundTrip(MaterialOverrides()).IsEmpty());
}

TEST_CASE("A tint override replaces the material's tint, and is saved and loaded")
{
	MaterialValues base;
	base.Tint = Vector3D(1.0f, 1.0f, 1.0f);

	MaterialOverrides overrides;
	overrides.Tint = Vector3D(0.5f, 0.25f, 0.0f);
	CHECK(overrides.ChangesValues());
	CHECK(overrides.ApplyTo(base).Tint.Y() == doctest::Approx(0.25f));

	const MaterialOverrides loaded = RoundTrip(overrides);
	REQUIRE(loaded.Tint.has_value());
	CHECK(loaded.Tint->X() == doctest::Approx(0.5f));
	CHECK(loaded.Tint->Y() == doctest::Approx(0.25f));
	CHECK(loaded.Tint->Z() == doctest::Approx(0.0f));

	// Anything but three numbers isn't a tint
	rapidjson::Document document;
	document.Parse(R"({ "Tint": [ 1, 0 ] })");
	REQUIRE_FALSE(document.HasParseError());
	CHECK_FALSE(MaterialOverrides::Deserialize(document).Tint.has_value());
}

TEST_CASE("Loading overrides ignores names it doesn't know")
{
	rapidjson::Document document;
	document.Parse(R"({ "BlendMode": "Sparkly", "Opacity": 0.5, "Textures": { "Glow": "Assets/Textures/Glow.png" }, "Colour": 3 })");
	REQUIRE_FALSE(document.HasParseError());

	const MaterialOverrides loaded = MaterialOverrides::Deserialize(document);
	CHECK_FALSE(loaded.BlendMode.has_value());
	CHECK(loaded.Opacity == doctest::Approx(0.5f));
	for (const std::optional<Filepath>& texture : loaded.Textures)
	{
		CHECK_FALSE(texture.has_value());
	}
}

TEST_CASE("Swapping in a material is an override of its own, kept and saved, that leaves the values to that material")
{
	MaterialOverrides overrides;
	overrides.Material = GetMaterialPath("Other");
	CHECK_FALSE(overrides.IsEmpty());
	CHECK_FALSE(overrides.ChangesValues());

	MaterialValues base;
	base.Shininess = 16.0f;
	CHECK(overrides.ApplyTo(base).Shininess == doctest::Approx(16.0f));

	overrides.Opacity = 0.5f;
	CHECK(overrides.ChangesValues());

	const MaterialOverrides loaded = RoundTrip(overrides);
	REQUIRE(loaded.Material.has_value());
	CHECK(loaded.Material->GetRelativePath() == GetMaterialPath("Other").GetRelativePath());
	CHECK(loaded.Opacity == doctest::Approx(0.5f));

	// An empty path isn't a material
	rapidjson::Document document;
	document.Parse(R"({ "Material": "" })");
	REQUIRE_FALSE(document.HasParseError());
	CHECK(MaterialOverrides::Deserialize(document).IsEmpty());
}

TEST_CASE("A slot draws its mesh's material asset until its overrides swap another in")
{
	std::shared_ptr<StaticMeshInstance> instance = MakeInstance({ "glass", "bricks" });
	const MaterialSlot& glass = instance->GetMaterialSlots()[0];
	CHECK(glass.GetMaterialPath().GetRelativePath() == GetMaterialPath("glass").GetRelativePath());

	MaterialOverridesByName overrides;
	overrides["glass"].Material = GetMaterialPath("Other");
	instance->SetOverrides(overrides);
	CHECK(glass.GetMaterialPath().GetRelativePath() == GetMaterialPath("Other").GetRelativePath());
	// Kept with the others, though no value is overridden
	CHECK(instance->GetOverrides().count("glass") == 1);

	instance->SetOverrides(MaterialOverridesByName());
	CHECK(glass.GetMaterialPath().GetRelativePath() == GetMaterialPath("glass").GetRelativePath());
	CHECK(instance->GetOverrides().empty());
}

TEST_CASE("A mesh instance's overrides go onto its slots by name, and ones for slots it hasn't got are kept")
{
	std::shared_ptr<StaticMeshInstance> instance = MakeInstance({ "glass", "bricks" });

	MaterialOverridesByName overrides;
	overrides["bricks"] = WithOpacity(0.5f);
	overrides["missing"] = WithOpacity(0.25f);
	instance->SetOverrides(overrides);

	const std::vector<MaterialSlot>& slots = instance->GetMaterialSlots();
	CHECK(slots[0].GetOverrides().IsEmpty());
	CHECK(slots[1].GetOverrides().Opacity == doctest::Approx(0.5f));
	CHECK(slots[1].GetValues().Opacity == doctest::Approx(0.5f));

	const MaterialOverridesByName read = instance->GetOverrides();
	CHECK(read.size() == 2);
	CHECK(read.count("bricks") == 1);
	CHECK(read.count("missing") == 1);
	CHECK(read.count("glass") == 0);

	// Replacing them clears what they don't name, kept ones included
	instance->SetOverrides(MaterialOverridesByName());
	CHECK(instance->GetOverrides().empty());
	CHECK(slots[1].GetOverrides().IsEmpty());
}

TEST_CASE("Swapping a component's mesh carries its overrides over by slot name, unless told to use the new instance's")
{
	RenderComponent component;
	std::shared_ptr<StaticMeshInstance> first = MakeInstance({ "glass", "bricks" });
	component.SetMeshInstance(first);

	MaterialOverridesByName overrides;
	overrides["glass"] = WithOpacity(0.3f);
	overrides["bricks"] = WithOpacity(0.6f);
	first->SetOverrides(overrides);

	// A mesh with only one of the slots: the other's override is kept for later
	std::shared_ptr<StaticMeshInstance> second = MakeInstance({ "glass" });
	component.SetMeshInstance(second);
	REQUIRE(component.GetMesh() == second.get());
	CHECK(second->GetMaterialSlots()[0].GetOverrides().Opacity == doctest::Approx(0.3f));
	CHECK(second->GetOverrides().count("bricks") == 1);

	// And back: both again
	std::shared_ptr<StaticMeshInstance> third = MakeInstance({ "glass", "bricks" });
	component.SetMeshInstance(third);
	CHECK(third->GetMaterialSlots()[1].GetOverrides().Opacity == doctest::Approx(0.6f));

	// A prepared instance keeps its own
	std::shared_ptr<StaticMeshInstance> prepared = MakeInstance({ "glass" });
	MaterialOverridesByName preparedOverrides;
	preparedOverrides["glass"] = WithOpacity(0.9f);
	prepared->SetOverrides(preparedOverrides);
	component.SetMeshInstance(prepared, RenderComponent::EMaterialOverrides::UseNew);
	CHECK(prepared->GetMaterialSlots()[0].GetOverrides().Opacity == doctest::Approx(0.9f));
	CHECK(prepared->GetOverrides().count("bricks") == 0);

	component.SetMeshInstance(nullptr);
}
