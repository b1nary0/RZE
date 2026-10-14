#include <doctest/doctest.h>

#include <Asset/AssetImport/SourceMaterialRules.h>

using SourceMaterialRules::SourceTransparency;
using EBlendMode = MaterialAssetImporter::MaterialData::EBlendMode;

TEST_CASE("A source that says nothing about transparency is solid")
{
	CHECK(SourceMaterialRules::ChooseOpacity(SourceTransparency()) == doctest::Approx(1.0f));
}

TEST_CASE("A declared opacity wins over every other transparency value (OBJ)")
{
	SourceTransparency source;
	source.Opacity = 0.8f;
	source.TransparencyFactor = 0.9f;
	source.MaxPhysicalTransparency = 1.0f;
	CHECK(SourceMaterialRules::ChooseOpacity(source) == doctest::Approx(0.8f));
}

TEST_CASE("FBX's transparency factor is used before 3ds Max's physical transparency")
{
	// New Sponza's glass: physically fully transmissive, 0.57 transparent in the legacy factor
	SourceTransparency glass;
	glass.TransparencyFactor = 0.568f;
	glass.MaxPhysicalTransparency = 1.0f;
	CHECK(SourceMaterialRules::ChooseOpacity(glass) == doctest::Approx(0.432f));

	SourceTransparency physicalOnly;
	physicalOnly.MaxPhysicalTransparency = 0.25f;
	CHECK(SourceMaterialRules::ChooseOpacity(physicalOnly) == doctest::Approx(0.75f));
}

TEST_CASE("Opacity stays within 0 and 1 whatever the source says")
{
	SourceTransparency tooTransparent;
	tooTransparent.TransparencyFactor = 1.5f;
	CHECK(SourceMaterialRules::ChooseOpacity(tooTransparent) == doctest::Approx(0.0f));

	SourceTransparency tooOpaque;
	tooOpaque.Opacity = 2.0f;
	CHECK(SourceMaterialRules::ChooseOpacity(tooOpaque) == doctest::Approx(1.0f));
}

TEST_CASE("A cutout mask makes a cutout even when the material is also partly see-through")
{
	// Nyra's cornea: map_d with d 0.12
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::Cutout, 0.12f) == EBlendMode::Cutout);
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::Cutout, 1.0f) == EBlendMode::Cutout);
}

TEST_CASE("A coverage mask blends, even at full opacity")
{
	// New Sponza's dirt_decal: transparency_map with opacity 0.61
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::Coverage, 0.61f) == EBlendMode::Transparent);
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::Coverage, 1.0f) == EBlendMode::Transparent);
}

TEST_CASE("Without a mask, anything short of solid blends")
{
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::None, 0.43f) == EBlendMode::Transparent);
	CHECK(SourceMaterialRules::ChooseBlendMode(SourceMaterialRules::EMaskKind::None, 1.0f) == EBlendMode::Opaque);
}
