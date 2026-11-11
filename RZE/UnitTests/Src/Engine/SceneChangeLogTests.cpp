#include <doctest/doctest.h>

#include <Graphics/SceneChangeLog.h>

#include <Utils/Math/BoundingBox.h>

#include <vector>

namespace
{
	// A metre-wide box with its lowest corner at corner
	BoundingBox MakeBox(const Vector3D& corner)
	{
		return BoundingBox(corner, corner + Vector3D(1.0f));
	}
}

TEST_CASE("A scene change log tells which regions changed after a revision")
{
	SceneChangeLog log;
	const BoundingBox nearOrigin(Vector3D(-2.0f), Vector3D(2.0f));
	const BoundingBox farAway(Vector3D(50.0f), Vector3D(60.0f));

	CHECK_FALSE(log.HasChangedWithin(nearOrigin, 0));

	log.Add(3, MakeBox(Vector3D(0.0f)));
	log.Add(5, MakeBox(Vector3D(55.0f)));

	CHECK(log.HasChangedWithin(nearOrigin, 0));
	CHECK(log.HasChangedWithin(nearOrigin, 2));
	// Changes at the revision asked about were already seen
	CHECK_FALSE(log.HasChangedWithin(nearOrigin, 3));
	CHECK(log.HasChangedWithin(farAway, 4));
	CHECK_FALSE(log.HasChangedWithin(farAway, 5));

	// Nothing changed between them
	CHECK_FALSE(log.HasChangedWithin(BoundingBox(Vector3D(10.0f), Vector3D(20.0f)), 0));
	CHECK_FALSE(log.HasChangedWithin(BoundingBox(), 0));
}

TEST_CASE("A scene change log ignores empty bounds")
{
	SceneChangeLog log;
	log.Add(1, BoundingBox());
	CHECK_FALSE(log.HasChangedWithin(BoundingBox(Vector3D(-1000.0f), Vector3D(1000.0f)), 0));
}

TEST_CASE("Once a scene change log is full, it merges the neighbouring changes closest together")
{
	SceneChangeLog log;

	// Ten metres apart, but the changes at revisions 6 and 7 are next to each other
	for (U64 revision = 1; revision <= SceneChangeLog::k_capacity; ++revision)
	{
		const float x = revision == 7 ? 51.0f : static_cast<float>(revision) * 10.0f - 10.0f;
		log.Add(revision, MakeBox(Vector3D(x, 0.0f, 0.0f)));
	}
	log.Add(SceneChangeLog::k_capacity + 1, MakeBox(Vector3D(-1000.0f)));

	// Nothing is forgotten, and the rest are still told apart
	CHECK_FALSE(log.HasChangedWithin(MakeBox(Vector3D(-500.0f)), 0));
	CHECK(log.HasChangedWithin(MakeBox(Vector3D(0.0f)), 0));
	CHECK_FALSE(log.HasChangedWithin(MakeBox(Vector3D(5.0f, 0.0f, 0.0f)), 0));

	// The two merged into one at the newer revision, so it counts for anything that hadn't seen both
	CHECK(log.HasChangedWithin(MakeBox(Vector3D(50.0f, 0.0f, 0.0f)), 6));
	CHECK_FALSE(log.HasChangedWithin(MakeBox(Vector3D(50.0f, 0.0f, 0.0f)), 7));

	U32 visited = 0;
	CHECK(log.ForEachChangeSince(0, [&visited](const BoundingBox&) { ++visited; }));
	CHECK(visited == SceneChangeLog::k_capacity);
}

TEST_CASE("A scene change log doesn't keep a big old change alive by merging new ones into it")
{
	SceneChangeLog log;
	// e.g. the whole scene, from loading it
	log.Add(1, BoundingBox(Vector3D(-50.0f), Vector3D(50.0f)));

	// Far more than fit, all in one corner, as when many objects move together every frame
	const BoundingBox corner = MakeBox(Vector3D(40.0f));
	for (U64 revision = 2; revision <= SceneChangeLog::k_capacity * 4; ++revision)
	{
		log.Add(revision, corner);
	}

	// What saw the load only sees the corner change since
	CHECK_FALSE(log.HasChangedWithin(MakeBox(Vector3D(-10.0f)), 1));
	CHECK(log.HasChangedWithin(corner, SceneChangeLog::k_capacity * 4 - 1));
}

TEST_CASE("A scene change log doesn't give a big old change a new change's revision")
{
	SceneChangeLog log;

	// Loading a scene: the log fills up and merges into a few big boxes, the last one around where the new changes are
	for (U64 revision = 1; revision <= SceneChangeLog::k_capacity * 4; ++revision)
	{
		const float x = static_cast<float>(revision % 64) * 2.0f;
		log.Add(revision, MakeBox(Vector3D(x, 0.0f, static_cast<float>(revision / 64) * 2.0f)));
	}
	const U64 loaded = SceneChangeLog::k_capacity * 4;

	// Something small moving a little, several times, where the last of the loading was
	const Vector3D corner(126.0f, 0.0f, 30.0f);
	for (U64 revision = loaded + 1; revision <= loaded + 20; ++revision)
	{
		log.Add(revision, MakeBox(corner));
		log.Add(revision, MakeBox(corner + Vector3D(0.1f, 0.0f, 0.0f)));
	}

	// What saw the loading only sees the small change
	CHECK(log.HasChangedWithin(MakeBox(corner), loaded));
	for (float x = 0.0f; x < 120.0f; x += 2.0f)
	{
		CHECK_FALSE(log.HasChangedWithin(MakeBox(Vector3D(x, 0.0f, 30.0f)), loaded));
	}
}

TEST_CASE("A scene change log lists the changes after a revision, newest first")
{
	SceneChangeLog log;
	log.Add(2, MakeBox(Vector3D(0.0f)));
	log.Add(2, MakeBox(Vector3D(10.0f)));
	log.Add(5, MakeBox(Vector3D(20.0f)));

	std::vector<float> corners;
	const auto visit = [&corners](const BoundingBox& bounds) { corners.push_back(bounds.GetMin().X()); };

	CHECK(log.ForEachChangeSince(0, visit));
	CHECK(corners == std::vector<float>{ 20.0f, 10.0f, 0.0f });

	// Changes at the revision asked about were already seen
	corners.clear();
	CHECK(log.ForEachChangeSince(2, visit));
	CHECK(corners == std::vector<float>{ 20.0f });

	corners.clear();
	CHECK(log.ForEachChangeSince(5, visit));
	CHECK(corners.empty());
}

TEST_CASE("A scene change log can't list changes from before a change everywhere")
{
	SceneChangeLog log;
	log.Add(1, MakeBox(Vector3D(0.0f)));
	log.AddEverywhere(3);
	log.Add(4, MakeBox(Vector3D(10.0f)));

	U32 visited = 0;
	const auto visit = [&visited](const BoundingBox&) { ++visited; };

	CHECK_FALSE(log.ForEachChangeSince(2, visit));
	CHECK(visited == 0);

	CHECK(log.ForEachChangeSince(3, visit));
	CHECK(visited == 1);
}

TEST_CASE("A change everywhere counts as a change in every region, for revisions before it")
{
	SceneChangeLog log;
	const BoundingBox region(Vector3D(-1.0f), Vector3D(1.0f));

	log.Add(1, MakeBox(Vector3D(30.0f)));
	log.AddEverywhere(4);

	CHECK(log.HasChangedWithin(region, 3));
	CHECK_FALSE(log.HasChangedWithin(region, 4));

	// Later changes are still told apart by region
	log.Add(6, MakeBox(Vector3D(30.0f)));
	CHECK_FALSE(log.HasChangedWithin(region, 5));
	CHECK(log.HasChangedWithin(MakeBox(Vector3D(30.0f)), 5));
}
