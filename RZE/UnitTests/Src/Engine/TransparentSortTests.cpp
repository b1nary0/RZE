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

TEST_CASE("Sorting no transparent draws is fine")
{
	std::vector<ForwardRenderStage::TransparentDraw> draws;
	ForwardRenderStage::SortBackToFront(draws);
	CHECK(draws.empty());
}
