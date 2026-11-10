#include <doctest/doctest.h>

#include <Graphics/RenderStages/OutlineRenderStage.h>
#include <Graphics/RenderView.h>

#include <limits>
#include <tuple>
#include <vector>

namespace
{
	using OutlinedObject = OutlineRenderStage::OutlinedObject;
	// LiveID, OutlineID, ShapeID: comparable, so a check prints what differs
	using Collected = std::tuple<U64, U32, U32>;

	RenderOutline MakeOutline(std::vector<RenderOutlineObject>&& objects)
	{
		RenderOutline outline;
		outline.Objects = std::move(objects);
		return outline;
	}

	std::vector<Collected> Collect(const std::vector<RenderOutline>& outlines, U32 outlineCount, bool& outHasAllShapes)
	{
		std::vector<OutlinedObject> objects;
		outHasAllShapes = OutlineRenderStage::CollectOutlinedObjects(outlines, outlineCount, objects);

		std::vector<Collected> collected;
		for (const OutlinedObject& object : objects)
		{
			collected.emplace_back(object.LiveID, object.OutlineID, object.ShapeID);
		}
		return collected;
	}

	std::vector<Collected> Collect(const std::vector<RenderOutline>& outlines)
	{
		bool hasAllShapes = false;
		std::vector<Collected> collected = Collect(outlines, static_cast<U32>(outlines.size()), hasAllShapes);
		CHECK(hasAllShapes);
		return collected;
	}
}

TEST_CASE("Outlined objects: each outline's shapes are numbered after the one before's")
{
	const std::vector<RenderOutline> outlines =
	{
		MakeOutline({ { 30, 0 }, { 20, 1 }, { 10, 0 } }),
		// Shape 1 is unused, but still numbered: shapes are numbered from 0 in each outline
		MakeOutline({ { 40, 0 }, { 50, 2 } }),
	};

	// Sorted by LiveID
	CHECK(Collect(outlines) == std::vector<Collected>{ { 10, 1, 1 }, { 20, 1, 2 }, { 30, 1, 1 }, { 40, 2, 3 }, { 50, 2, 5 } });
}

TEST_CASE("Outlined objects: an object listed more than once is in the first outline, in its lowest shape there")
{
	const std::vector<RenderOutline> outlines =
	{
		MakeOutline({ { 7, 1 }, { 8, 0 }, { 7, 0 } }),
		MakeOutline({ { 7, 0 } }),
	};

	CHECK(Collect(outlines) == std::vector<Collected>{ { 7, 1, 1 }, { 8, 1, 1 } });
}

TEST_CASE("Outlined objects: an outline with no objects takes no shape numbers")
{
	const std::vector<RenderOutline> outlines =
	{
		MakeOutline({}),
		MakeOutline({ { 1, 0 } }),
	};

	CHECK(Collect(outlines) == std::vector<Collected>{ { 1, 2, 1 } });
}

TEST_CASE("Outlined objects: only the first outlineCount outlines are collected")
{
	const std::vector<RenderOutline> outlines =
	{
		MakeOutline({ { 1, 0 } }),
		MakeOutline({ { 2, 0 } }),
	};

	bool hasAllShapes = false;
	CHECK(Collect(outlines, 1, hasAllShapes) == std::vector<Collected>{ { 1, 1, 1 } });
	CHECK(hasAllShapes);
}

TEST_CASE("Outlined objects: shapes past k_maxShapes share the last ID")
{
	constexpr U32 last = OutlineRenderStage::k_maxShapes;
	bool hasAllShapes = false;

	SUBCASE("The last ID itself fits")
	{
		const std::vector<RenderOutline> outlines = { MakeOutline({ { 1, last - 1 } }) };
		CHECK(Collect(outlines, 1, hasAllShapes) == std::vector<Collected>{ { 1, 1, last } });
		CHECK(hasAllShapes);
	}

	SUBCASE("The next outline's shapes are past it")
	{
		const std::vector<RenderOutline> outlines =
		{
			MakeOutline({ { 1, last - 1 } }),
			MakeOutline({ { 2, 0 }, { 3, 1 } }),
		};
		CHECK(Collect(outlines, 2, hasAllShapes) == std::vector<Collected>{ { 1, 1, last }, { 2, 2, last }, { 3, 2, last } });
		CHECK_FALSE(hasAllShapes);
	}

	SUBCASE("Any U32 Shape, without wrapping round")
	{
		const std::vector<RenderOutline> outlines =
		{
			MakeOutline({ { 1, std::numeric_limits<U32>::max() } }),
			MakeOutline({ { 2, std::numeric_limits<U32>::max() } }),
		};
		CHECK(Collect(outlines, 2, hasAllShapes) == std::vector<Collected>{ { 1, 1, last }, { 2, 2, last } });
		CHECK_FALSE(hasAllShapes);
	}
}

TEST_CASE("Outlined objects: what was collected before is replaced")
{
	std::vector<OutlinedObject> objects = { { 99, 1, 1 } };
	const std::vector<RenderOutline> outlines = { MakeOutline({ { 1, 0 } }) };

	CHECK(OutlineRenderStage::CollectOutlinedObjects(outlines, 1, objects));

	REQUIRE(objects.size() == 1);
	CHECK(objects.front().LiveID == 1);
}
