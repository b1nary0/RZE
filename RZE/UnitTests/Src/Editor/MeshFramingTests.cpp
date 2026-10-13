#include <doctest/doctest.h>

#include <Services/MeshFraming.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

using namespace Editor;

namespace
{
	// Whether every corner of the box is in the camera's view and between its near and far planes
	bool IsBoxInView(const RenderCamera& camera, const Vector3D& boundsMin, const Vector3D& boundsMax)
	{
		for (int corner = 0; corner < 8; ++corner)
		{
			const Vector4D clip = camera.ClipSpace * Vector4D(
				(corner & 1) ? boundsMax.X() : boundsMin.X(),
				(corner & 2) ? boundsMax.Y() : boundsMin.Y(),
				(corner & 4) ? boundsMax.Z() : boundsMin.Z(),
				1.0f);

			// D3D clip space: x and y within [-w, w], depth within [0, w]
			const bool isInside = clip.W() > 0.0f
				&& clip.X() >= -clip.W() && clip.X() <= clip.W()
				&& clip.Y() >= -clip.W() && clip.Y() <= clip.W()
				&& clip.Z() >= 0.0f && clip.Z() <= clip.W();
			if (!isInside)
			{
				return false;
			}
		}
		return true;
	}
}

TEST_CASE("FrameBounds keeps the whole box in view, for boxes of any shape and place")
{
	struct Box
	{
		Vector3D Min;
		Vector3D Max;
	};

	const Box boxes[] =
	{
		{ Vector3D(-0.5f, -0.5f, -0.5f), Vector3D(0.5f, 0.5f, 0.5f) },
		// Tall and thin, like a character
		{ Vector3D(-0.3f, 0.0f, -0.2f), Vector3D(0.3f, 1.8f, 0.2f) },
		// Wide and flat, like a building, away from the origin
		{ Vector3D(-40.0f, 0.0f, -20.0f), Vector3D(20.0f, 15.0f, 25.0f) },
		{ Vector3D(100.0f, 5.0f, -300.0f), Vector3D(101.0f, 6.0f, -299.0f) },
	};

	for (const Box& box : boxes)
	{
		CHECK(IsBoxInView(MeshFraming::FrameBounds(box.Min, box.Max, 40.0f, Vector2D(512.0f, 512.0f)), box.Min, box.Max));
		// A wide view is limited by its height, a tall one by its width
		CHECK(IsBoxInView(MeshFraming::FrameBounds(box.Min, box.Max, 40.0f, Vector2D(1024.0f, 256.0f)), box.Min, box.Max));
		CHECK(IsBoxInView(MeshFraming::FrameBounds(box.Min, box.Max, 40.0f, Vector2D(256.0f, 1024.0f)), box.Min, box.Max));
	}
}

TEST_CASE("FrameBounds looks from in front, to the right and above")
{
	const RenderCamera camera = MeshFraming::FrameBounds(Vector3D(-1.0f, -1.0f, -1.0f), Vector3D(1.0f, 1.0f, 1.0f), 40.0f, Vector2D(512.0f, 512.0f));

	CHECK(camera.Position.X() > 0.0f);
	CHECK(camera.Position.Y() > 0.0f);
	CHECK(camera.Position.Z() > 0.0f);
}

TEST_CASE("FrameBounds still frames a box with no size")
{
	const Vector3D point(2.0f, 3.0f, 4.0f);
	const RenderCamera camera = MeshFraming::FrameBounds(point, point, 40.0f, Vector2D(512.0f, 512.0f));

	CHECK(IsBoxInView(camera, point, point));
	CHECK((camera.Position - point).Length() > 0.0f);
}
