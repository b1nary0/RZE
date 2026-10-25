#include <doctest/doctest.h>

#include <Graphics/SceneChangeLog.h>

#include <Utils/Math/BoundingBox.h>

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

TEST_CASE("Once a scene change log is full, revisions from before what it kept count as changed everywhere")
{
	SceneChangeLog log;
	const BoundingBox elsewhere(Vector3D(-10.0f), Vector3D(-5.0f));

	for (U64 revision = 1; revision <= SceneChangeLog::k_capacity; ++revision)
	{
		log.Add(revision, MakeBox(Vector3D(0.0f)));
	}
	CHECK_FALSE(log.HasChangedWithin(elsewhere, 0));

	// Revision 1 is forgotten
	log.Add(SceneChangeLog::k_capacity + 1, MakeBox(Vector3D(0.0f)));
	CHECK(log.HasChangedWithin(elsewhere, 0));
	CHECK_FALSE(log.HasChangedWithin(elsewhere, 1));
	CHECK_FALSE(log.HasChangedWithin(elsewhere, 2));
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
