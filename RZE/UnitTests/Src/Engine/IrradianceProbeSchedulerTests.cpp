#include <doctest/doctest.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeScheduler.h>
#include <Graphics/RenderScene.h>
#include <Graphics/StaticMesh.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>

#include <cmath>
#include <memory>
#include <set>
#include <vector>

// Meshes are built on the CPU, as in RenderSceneBoundsTests: their vertices are enough for their bounds

namespace
{
	// An instance of a new mesh with one sub-mesh around box
	std::shared_ptr<StaticMeshInstance> MakeBoxMesh(const BoundingBox& box)
	{
		StaticMesh::SubMesh subMesh;
		for (const Vector3D& corner : box.GetCorners())
		{
			MeshVertex vertex;
			vertex.Position = corner;
			subMesh.Geometry.AddVertex(vertex);
		}

		std::vector<StaticMesh::SubMesh> subMeshes;
		subMeshes.push_back(std::move(subMesh));
		std::vector<StaticMesh::Slot> slots(1);
		slots[0].Name = "Slot0";
		return StaticMeshInstance::Create(StaticMesh::Create("Test", std::move(subMeshes), std::move(slots)));
	}

	// A 40 x 10 x 40 m room, with a 1 m prop inside it that the tests move
	struct ProbeScene
	{
		RenderScene Scene;
		RenderObjectPtr Room;
		RenderObjectPtr Prop;

		ProbeScene()
		{
			Room = Scene.CreateRenderObject(MakeBoxMesh(BoundingBox(Vector3D(-20.0f, 0.0f, -20.0f), Vector3D(20.0f, 10.0f, 20.0f))));
			Prop = Scene.CreateRenderObject(MakeBoxMesh(BoundingBox(Vector3D(-0.5f), Vector3D(0.5f))));
			MoveProp(Vector3D(-15.0f, 1.0f, -15.0f));
		}

		~ProbeScene()
		{
			Scene.DestroyRenderObject(Prop);
			Scene.DestroyRenderObject(Room);
		}

		void MoveProp(const Vector3D& position)
		{
			Prop->SetTransform(Matrix4x4::CreateInPlace(position, Vector3D(1.0f), Vector3D()));
		}
	};

	constexpr U32 k_targetProbes = 1024;

	// Takes and captures every waiting probe
	void CaptureAll(IrradianceProbeScheduler& scheduler)
	{
		while (scheduler.HasCaptureWaiting())
		{
			scheduler.MarkCaptured(scheduler.TakeCapture());
		}
	}
}

TEST_CASE_FIXTURE(ProbeScene, "The probe grid is fitted to the scene, and again only when its bounds move by more than a spacing")
{
	IrradianceProbeScheduler scheduler;
	CHECK(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));

	const IrradianceProbeGrid& grid = scheduler.GetGrid();
	REQUIRE_FALSE(grid.IsEmpty());
	CHECK(scheduler.HasCaptureWaiting());
	CHECK_FALSE(scheduler.IsCaptured(0));

	CHECK_FALSE(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));

	// The prop pokes out of the room by less than a spacing, then by more
	MoveProp(Vector3D(20.0f + grid.Spacing * 0.5f, 1.0f, 0.0f));
	CHECK_FALSE(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));
	MoveProp(Vector3D(20.0f + grid.Spacing * 2.0f, 1.0f, 0.0f));
	CHECK(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));
}

TEST_CASE_FIXTURE(ProbeScene, "Probes are captured nearest the camera first")
{
	IrradianceProbeScheduler scheduler;
	const Vector3D camera(10.0f, 2.0f, 5.0f);
	REQUIRE(scheduler.FitGrid(Scene, k_targetProbes, camera));

	const IrradianceProbeGrid& grid = scheduler.GetGrid();
	float lastDistance = 0.0f;
	U32 taken = 0;
	while (scheduler.HasCaptureWaiting())
	{
		const float distance = (grid.GetProbePosition(scheduler.TakeCapture()) - camera).Length();
		CHECK(distance >= lastDistance - 0.0001f);
		lastDistance = distance;
		++taken;
	}
	CHECK(taken == grid.GetProbeCount());
}

TEST_CASE_FIXTURE(ProbeScene, "Moving an object queues the captured probes within five spacings of it, and no others")
{
	IrradianceProbeScheduler scheduler;
	REQUIRE(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));
	const IrradianceProbeGrid& grid = scheduler.GetGrid();

	// Not captured yet: already waiting, so not queued again
	MoveProp(Vector3D(-14.0f, 1.0f, -15.0f));
	scheduler.QueueChangedProbes(Scene, Vector3D());
	U32 waiting = 0;
	for (IrradianceProbeScheduler copy = scheduler; copy.HasCaptureWaiting(); copy.TakeCapture())
	{
		++waiting;
	}
	CHECK(waiting == grid.GetProbeCount());

	CaptureAll(scheduler);
	CHECK_FALSE(scheduler.HasCaptureWaiting());

	// Where it was and where it is, each grown by five spacings
	const BoundingBox before = Scene.GetRenderObjects()[1]->GetBounds();
	MoveProp(Vector3D(-13.0f, 1.0f, -15.0f));
	const BoundingBox after = Scene.GetRenderObjects()[1]->GetBounds();
	scheduler.QueueChangedProbes(Scene, Vector3D());

	const Vector3D reach(grid.Spacing * 5.0f);
	std::set<U32> queued;
	while (scheduler.HasCaptureWaiting())
	{
		const U32 probe = scheduler.TakeCapture();
		const Vector3D position = grid.GetProbePosition(probe);
		const BoundingBox probeReach(position - reach, position + reach);
		CHECK((probeReach.Intersects(before) || probeReach.Intersects(after)));
		queued.insert(probe);
	}
	CHECK_FALSE(queued.empty());
	CHECK(queued.size() < grid.GetProbeCount());

	// Nothing changed since
	scheduler.QueueChangedProbes(Scene, Vector3D());
	CHECK_FALSE(scheduler.HasCaptureWaiting());
}

TEST_CASE_FIXTURE(ProbeScene, "Relighting lists the probes just captured, then those around the camera, then the rest in turn")
{
	IrradianceProbeScheduler scheduler;
	const Vector3D camera(0.0f, 2.0f, 0.0f);
	REQUIRE(scheduler.FitGrid(Scene, k_targetProbes, camera));
	CaptureAll(scheduler);
	const IrradianceProbeGrid& grid = scheduler.GetGrid();

	// Two just captured, far from the camera
	GraphicsLayouts::ProbeListLayout list;
	list.Probes[list.Count++] = 0;
	list.Probes[list.Count++] = grid.GetProbeCount() - 1;
	scheduler.ListRelights(200, camera, 1, list);

	REQUIRE(list.Count == 200);
	CHECK(list.Probes[0] == 0);
	CHECK(list.Probes[1] == grid.GetProbeCount() - 1);

	// Then the 5 x 5 x 5 around the camera, as far as the grid goes
	const Vector3D cameraCell = (camera - grid.Origin) / grid.Spacing;
	for (U32 slot = 2; slot < 2 + 5 * 5 * 5 && slot < list.Count; ++slot)
	{
		const Vector3D cell = (grid.GetProbePosition(list.Probes[slot]) - grid.Origin) / grid.Spacing;
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(std::abs(cell[axis] - std::round(cameraCell[axis])) <= 2.0f + 0.001f);
		}
	}

	// Each at most once a frame
	const std::set<U32> unique(list.Probes, list.Probes + list.Count);
	CHECK(unique.size() == list.Count);

	// In turn, every probe is relit: each frame's budget, less the probes around the camera, goes to the rest
	std::set<U32> relit(unique);
	const U64 framesNeeded = grid.GetProbeCount() / (200 - 5 * 5 * 5) + 1;
	for (U64 frame = 2; frame < 2 + framesNeeded; ++frame)
	{
		GraphicsLayouts::ProbeListLayout next;
		scheduler.ListRelights(200, camera, frame, next);
		CHECK(next.Count == 200);
		relit.insert(next.Probes, next.Probes + next.Count);
	}
	CHECK(relit.size() == grid.GetProbeCount());
}

TEST_CASE_FIXTURE(ProbeScene, "Only captured probes are relit, and no more than a probe list holds")
{
	IrradianceProbeScheduler scheduler;
	REQUIRE(scheduler.FitGrid(Scene, k_targetProbes, Vector3D()));

	GraphicsLayouts::ProbeListLayout none;
	scheduler.ListRelights(512, Vector3D(), 1, none);
	CHECK(none.Count == 0);

	CaptureAll(scheduler);
	GraphicsLayouts::ProbeListLayout capped;
	scheduler.ListRelights(100000, Vector3D(), 2, capped);
	CHECK(capped.Count == GraphicsLayouts::ProbeListLayout::k_maxProbes);
}
