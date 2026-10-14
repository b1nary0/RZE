#include <doctest/doctest.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>

TEST_CASE("A new bounding box is empty, and growing it by a point makes it just that point")
{
	BoundingBox box;
	CHECK(box.IsEmpty());

	box.Grow(Vector3D(1.0f, 2.0f, 3.0f));
	CHECK_FALSE(box.IsEmpty());
	CHECK(box.GetMin() == Vector3D(1.0f, 2.0f, 3.0f));
	CHECK(box.GetMax() == Vector3D(1.0f, 2.0f, 3.0f));
	CHECK(box.GetSize() == Vector3D(0.0f, 0.0f, 0.0f));
}

TEST_CASE("A bounding box grows to hold points and other boxes, but not empty ones")
{
	BoundingBox box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 1.0f, 1.0f));
	box.Grow(Vector3D(-1.0f, 0.5f, 4.0f));
	CHECK(box.GetMin() == Vector3D(-1.0f, 0.0f, 0.0f));
	CHECK(box.GetMax() == Vector3D(1.0f, 1.0f, 4.0f));

	box.Grow(BoundingBox(Vector3D(0.0f, -2.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)));
	CHECK(box.GetMin() == Vector3D(-1.0f, -2.0f, 0.0f));

	const BoundingBox before = box;
	box.Grow(BoundingBox());
	CHECK(box == before);

	BoundingBox empty;
	empty.Grow(before);
	CHECK(empty == before);
}

TEST_CASE("A bounding box's centre, size, radius and corners")
{
	const BoundingBox box(Vector3D(-1.0f, 0.0f, 2.0f), Vector3D(3.0f, 2.0f, 6.0f));

	CHECK(box.GetCentre() == Vector3D(1.0f, 1.0f, 4.0f));
	CHECK(box.GetSize() == Vector3D(4.0f, 2.0f, 4.0f));
	CHECK(box.GetRadius() == doctest::Approx(3.0f));

	const BoundingBox::Corners corners = box.GetCorners();
	CHECK(corners[0] == box.GetMin());
	CHECK(corners[7] == box.GetMax());
	// Bit 0 picks the maximum on X, bit 1 on Y, bit 2 on Z
	CHECK(corners[1] == Vector3D(3.0f, 0.0f, 2.0f));
	CHECK(corners[2] == Vector3D(-1.0f, 2.0f, 2.0f));
	CHECK(corners[4] == Vector3D(-1.0f, 0.0f, 6.0f));
	CHECK(BoundingBox::Around(corners) == box);
}

TEST_CASE("A transformed bounding box holds its transformed corners")
{
	const BoundingBox box(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(1.0f, 2.0f, 3.0f));

	const Matrix4x4 moveAndScale = Matrix4x4::CreateInPlace(Vector3D(10.0f, 0.0f, -5.0f), Vector3D(2.0f), Vector3D());
	const BoundingBox moved = box.Transformed(moveAndScale);
	CHECK(moved.GetMin().X() == doctest::Approx(10.0f));
	CHECK(moved.GetMin().Z() == doctest::Approx(-5.0f));
	CHECK(moved.GetMax().X() == doctest::Approx(12.0f));
	CHECK(moved.GetMax().Y() == doctest::Approx(4.0f));
	CHECK(moved.GetMax().Z() == doctest::Approx(1.0f));

	// A quarter turn about Y swaps the box's X and Z extents
	const Matrix4x4 quarterTurn = Matrix4x4::CreateInPlace(Vector3D(), Vector3D(1.0f), Vector3D(0.0f, 90.0f, 0.0f));
	const BoundingBox turned = box.Transformed(quarterTurn);
	CHECK(turned.GetSize().X() == doctest::Approx(3.0f));
	CHECK(turned.GetSize().Y() == doctest::Approx(2.0f));
	CHECK(turned.GetSize().Z() == doctest::Approx(1.0f));

	CHECK(BoundingBox().Transformed(moveAndScale).IsEmpty());
}

TEST_CASE("Transforming a bounding box gives the box around its transformed corners")
{
	const BoundingBox box(Vector3D(-1.0f, 0.5f, 2.0f), Vector3D(3.0f, 2.0f, 2.5f));
	const Matrix4x4 transforms[] =
	{
		Matrix4x4::CreateInPlace(Vector3D(4.0f, -2.0f, 7.0f), Vector3D(1.0f), Vector3D(30.0f, 45.0f, 10.0f)),
		Matrix4x4::CreateInPlace(Vector3D(), Vector3D(0.5f, 3.0f, 2.0f), Vector3D(-60.0f, 0.0f, 120.0f)),
	};

	for (const Matrix4x4& transform : transforms)
	{
		const BoundingBox placed = box.Transformed(transform);
		const BoundingBox aroundCorners = BoundingBox::Around(box.GetTransformedCorners(transform));
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(placed.GetMin()[axis] == doctest::Approx(aroundCorners.GetMin()[axis]).epsilon(0.0001));
			CHECK(placed.GetMax()[axis] == doctest::Approx(aroundCorners.GetMax()[axis]).epsilon(0.0001));
		}
	}
}
