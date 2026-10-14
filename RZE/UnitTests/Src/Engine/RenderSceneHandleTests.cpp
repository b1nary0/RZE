#include <doctest/doctest.h>

#include <Graphics/RenderScene.h>
#include <Graphics/StaticMeshInstance.h>

#include <memory>
#include <type_traits>
#include <utility>

// Lights need the renderer to be made (a constant buffer), so these use render objects. Both handles are the same
// template, so they behave the same.

static_assert(!std::is_copy_constructible_v<RenderObjectPtr> && !std::is_copy_assignable_v<RenderObjectPtr>, "One holder per render object");
static_assert(std::is_nothrow_move_constructible_v<RenderObjectPtr> && std::is_nothrow_move_assignable_v<RenderObjectPtr>, "Handles can be handed over");
static_assert(!std::is_copy_constructible_v<LightObjectPtr> && !std::is_copy_assignable_v<LightObjectPtr>, "One holder per light");
static_assert(std::is_nothrow_move_constructible_v<LightObjectPtr> && std::is_nothrow_move_assignable_v<LightObjectPtr>, "Handles can be handed over");

namespace
{
	// Nothing in these tests draws, so a mesh with no submeshes will do
	StaticMeshInstance MakeEmptyMesh()
	{
		StaticMeshInstance mesh;
		mesh.Initialize({});
		return mesh;
	}
}

TEST_CASE("Moving a render object handle hands the object over and leaves the old handle empty")
{
	RenderScene scene;
	RenderObjectPtr first = scene.CreateRenderObject(MakeEmptyMesh());
	const RenderObject* const object = first.operator->();

	RenderObjectPtr second = std::move(first);
	CHECK(first == nullptr);
	REQUIRE(second != nullptr);
	CHECK(second.operator->() == object);

	RenderObjectPtr third;
	third = std::move(second);
	CHECK(second == nullptr);
	CHECK(third.operator->() == object);

	scene.DestroyRenderObject(third);
}

TEST_CASE("Destroying a render object through its handle empties the handle")
{
	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(MakeEmptyMesh());
	REQUIRE(scene.GetRenderObjects().size() == 1);

	scene.DestroyRenderObject(object);

	CHECK(object == nullptr);
	CHECK(scene.GetRenderObjects().empty());
}

TEST_CASE("A render object handle knows its object is gone, however it went")
{
	RenderScene scene;
	RenderObjectPtr stale = scene.CreateRenderObject(MakeEmptyMesh());

	SUBCASE("The scene cleared its objects")
	{
		scene.ClearObjects();
		CHECK(stale == nullptr);
		CHECK_FALSE(stale.IsValid());
	}

	SUBCASE("The scene itself went")
	{
		RenderObjectPtr orphan;
		{
			RenderScene shortLived;
			orphan = shortLived.CreateRenderObject(MakeEmptyMesh());
			REQUIRE(orphan != nullptr);
		}
		CHECK(orphan == nullptr);

		scene.DestroyRenderObject(stale);
	}
}

TEST_CASE("Destroying through a handle whose object is already gone leaves newer objects alone")
{
	RenderScene scene;
	RenderObjectPtr stale = scene.CreateRenderObject(MakeEmptyMesh());
	scene.ClearObjects();

	// May well land at the stale handle's old address
	RenderObjectPtr fresh = scene.CreateRenderObject(MakeEmptyMesh());

	scene.DestroyRenderObject(stale);

	CHECK(stale == nullptr);
	CHECK(fresh != nullptr);
	CHECK(scene.GetRenderObjects().size() == 1);

	scene.DestroyRenderObject(fresh);
}

TEST_CASE("Handles to different render objects differ, and empty handles are equal")
{
	RenderScene scene;
	RenderObjectPtr a = scene.CreateRenderObject(MakeEmptyMesh());
	RenderObjectPtr b = scene.CreateRenderObject(MakeEmptyMesh());

	CHECK(a != b);
	CHECK(RenderObjectPtr() == RenderObjectPtr());

	scene.DestroyRenderObject(a);
	scene.DestroyRenderObject(b);
}
