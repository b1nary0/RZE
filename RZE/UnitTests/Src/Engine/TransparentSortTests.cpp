#include <doctest/doctest.h>

#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <vector>

TEST_CASE("Transparent draws sort farthest first, so nearer surfaces blend over farther ones")
{
	std::vector<ForwardRenderStage::TransparentDraw> draws(4);
	draws[0].DistanceSquared = 4.0f;
	draws[0].PlacementIndex = 0;
	draws[1].DistanceSquared = 100.0f;
	draws[1].PlacementIndex = 1;
	draws[2].DistanceSquared = 0.25f;
	draws[2].PlacementIndex = 2;
	draws[3].DistanceSquared = 36.0f;
	draws[3].PlacementIndex = 3;

	ForwardRenderStage::SortBackToFront(draws);

	REQUIRE(draws.size() == 4);
	CHECK(draws[0].PlacementIndex == 1);
	CHECK(draws[1].PlacementIndex == 3);
	CHECK(draws[2].PlacementIndex == 0);
	CHECK(draws[3].PlacementIndex == 2);
}

TEST_CASE("Each blended material mode draws with its own blend state; the others aren't blended")
{
	using EBlendMode = MaterialAssetImporter::MaterialData::EBlendMode;

	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Transparent) == Rendering::EBlendState::AlphaBlend);
	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Premultiplied) == Rendering::EBlendState::Premultiplied);
	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Additive) == Rendering::EBlendState::Additive);
	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Multiply) == Rendering::EBlendState::Multiply);
	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Opaque) == Rendering::EBlendState::Opaque);
	CHECK(ForwardRenderStage::GetBlendState(EBlendMode::Cutout) == Rendering::EBlendState::Opaque);

	CHECK_FALSE(MaterialAssetImporter::IsBlended(EBlendMode::Opaque));
	CHECK_FALSE(MaterialAssetImporter::IsBlended(EBlendMode::Cutout));
	CHECK(MaterialAssetImporter::IsBlended(EBlendMode::Transparent));
	CHECK(MaterialAssetImporter::IsBlended(EBlendMode::Premultiplied));
	CHECK(MaterialAssetImporter::IsBlended(EBlendMode::Additive));
	CHECK(MaterialAssetImporter::IsBlended(EBlendMode::Multiply));
}

TEST_CASE("Sorting no transparent draws is fine")
{
	std::vector<ForwardRenderStage::TransparentDraw> draws;
	ForwardRenderStage::SortBackToFront(draws);
	CHECK(draws.empty());
}
