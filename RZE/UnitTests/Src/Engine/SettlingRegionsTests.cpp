#include <doctest/doctest.h>

#include <Graphics/SettlingRegions.h>

#include <Utils/Math/BoundingBox.h>

#include <vector>

namespace
{
	// A metre-wide box with its lowest corner at corner
	BoundingBox MakeBox(const Vector3D& corner)
	{
		return BoundingBox(corner, corner + Vector3D(1.0f));
	}

	std::vector<BoundingBox> TakeSettled(SettlingRegions& regions, U64 frame, U64 settleFrames)
	{
		std::vector<BoundingBox> settled;
		regions.TakeSettled(frame, settleFrames, [&settled](const BoundingBox& box) { settled.push_back(box); });
		return settled;
	}
}

TEST_CASE("A region settles once nothing has changed in it for long enough, and only once")
{
	SettlingRegions regions;
	CHECK(regions.IsEmpty());

	regions.Touch(MakeBox(Vector3D(0.0f)), 10);
	CHECK(TakeSettled(regions, 10, 8).empty());
	CHECK(TakeSettled(regions, 17, 8).empty());

	const std::vector<BoundingBox> settled = TakeSettled(regions, 18, 8);
	REQUIRE(settled.size() == 1);
	CHECK(settled[0] == MakeBox(Vector3D(0.0f)));
	CHECK(regions.IsEmpty());
	CHECK(TakeSettled(regions, 30, 8).empty());
}

TEST_CASE("A region that keeps changing doesn't settle until it stops")
{
	SettlingRegions regions;
	for (U64 frame = 0; frame < 50; ++frame)
	{
		// Moving a little each frame, so each touch overlaps the last
		regions.Touch(MakeBox(Vector3D(static_cast<float>(frame) * 0.1f, 0.0f, 0.0f)), frame);
		CHECK(TakeSettled(regions, frame, 8).empty());
	}

	// Stopped at frame 49: one box around the whole way it went
	CHECK(TakeSettled(regions, 56, 8).empty());
	const std::vector<BoundingBox> settled = TakeSettled(regions, 57, 8);
	REQUIRE(settled.size() == 1);
	CHECK(settled[0].GetMin().X() == doctest::Approx(0.0f));
	CHECK(settled[0].GetMax().X() == doctest::Approx(5.9f));
}

TEST_CASE("Regions apart settle apart, and a touch joining two merges them")
{
	SettlingRegions regions;
	regions.Touch(MakeBox(Vector3D(0.0f)), 0);
	regions.Touch(MakeBox(Vector3D(10.0f, 0.0f, 0.0f)), 5);

	std::vector<BoundingBox> settled = TakeSettled(regions, 8, 8);
	REQUIRE(settled.size() == 1);
	CHECK(settled[0] == MakeBox(Vector3D(0.0f)));
	CHECK_FALSE(regions.IsEmpty());

	// One spanning both of two new ones: all one region, changed when it was touched
	regions.Touch(MakeBox(Vector3D(20.0f, 0.0f, 0.0f)), 9);
	regions.Touch(BoundingBox(Vector3D(10.5f, 0.0f, 0.0f), Vector3D(20.5f, 1.0f, 1.0f)), 12);
	CHECK(TakeSettled(regions, 19, 8).empty());
	settled = TakeSettled(regions, 20, 8);
	REQUIRE(settled.size() == 1);
	CHECK(settled[0] == BoundingBox(Vector3D(10.0f, 0.0f, 0.0f), Vector3D(21.0f, 1.0f, 1.0f)));

	// Settling straight away, and ignoring empty boxes
	regions.Touch(BoundingBox(), 30);
	CHECK(regions.IsEmpty());
	regions.Touch(MakeBox(Vector3D(0.0f)), 30);
	CHECK(TakeSettled(regions, 30, 0).size() == 1);
}
