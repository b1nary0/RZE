#include <doctest/doctest.h>

#include <Graphics/MeshDraw.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <vector>

TEST_CASE("Transparent draws sort farthest first, so nearer surfaces blend over farther ones")
{
	const std::vector<MeshDraw> meshDraws(4);
	std::vector<ForwardRenderStage::TransparentDraw> draws(4);
	const float distancesSquared[] = { 4.0f, 100.0f, 0.25f, 36.0f };
	for (size_t drawIndex = 0; drawIndex < draws.size(); ++drawIndex)
	{
		draws[drawIndex].DistanceSquared = distancesSquared[drawIndex];
		draws[drawIndex].Draw = &meshDraws[drawIndex];
	}

	ForwardRenderStage::SortBackToFront(draws);

	REQUIRE(draws.size() == 4);
	CHECK(draws[0].Draw == &meshDraws[1]);
	CHECK(draws[1].Draw == &meshDraws[3]);
	CHECK(draws[2].Draw == &meshDraws[0]);
	CHECK(draws[3].Draw == &meshDraws[2]);
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
