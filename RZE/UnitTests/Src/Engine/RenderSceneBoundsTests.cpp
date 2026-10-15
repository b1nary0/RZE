#include <doctest/doctest.h>

#include <Graphics/RenderScene.h>
#include <Graphics/StaticMesh.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>

#include <memory>
#include <string>
#include <vector>

// Meshes are built on the CPU here: their vertices are enough for their bounds, and nothing in these tests draws, so
// their slots have no material (making one needs the renderer)

namespace
{
	// A sub-mesh with a vertex at each corner of box, drawn with slot slotIndex
	StaticMesh::SubMesh MakeSubMesh(const BoundingBox& box, U32 slotIndex = 0)
	{
		StaticMesh::SubMesh subMesh;
		subMesh.SlotIndex = slotIndex;
		for (const Vector3D& corner : box.GetCorners())
		{
			MeshVertex vertex;
			vertex.Position = corner;
			subMesh.Geometry.AddVertex(vertex);
		}
		return subMesh;
	}

	// slotCount slots named "Slot0", "Slot1", ...
	std::vector<StaticMesh::Slot> MakeSlots(size_t slotCount)
	{
		std::vector<StaticMesh::Slot> slots(slotCount);
		for (size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex)
		{
			slots[slotIndex].Name = "Slot" + std::to_string(slotIndex);
		}
		return slots;
	}

	// An instance of a new mesh with a sub-mesh around each box, each placed at the mesh's origin and sharing one slot
	std::shared_ptr<StaticMeshInstance> MakeMesh(const std::vector<BoundingBox>& subMeshBoxes)
	{
		std::vector<StaticMesh::SubMesh> subMeshes;
		for (const BoundingBox& box : subMeshBoxes)
		{
			subMeshes.push_back(MakeSubMesh(box));
		}

		return StaticMeshInstance::Create(StaticMesh::Create("Test", std::move(subMeshes), MakeSlots(1)));
	}

	Matrix4x4 MakeTransform(const Vector3D& position, const Vector3D& rotation = Vector3D(), const Vector3D& scale = Vector3D(1.0f))
	{
		return Matrix4x4::CreateInPlace(position, scale, rotation);
	}

	void CheckBox(const BoundingBox& box, const Vector3D& expectedMin, const Vector3D& expectedMax)
	{
		REQUIRE_FALSE(box.IsEmpty());
		for (int axis = 0; axis < 3; ++axis)
		{
			CHECK(box.GetMin()[axis] == doctest::Approx(expectedMin[axis]));
			CHECK(box.GetMax()[axis] == doctest::Approx(expectedMax[axis]));
		}
	}

	const BoundingBox k_unitBox(Vector3D(-1.0f), Vector3D(1.0f));
}

TEST_CASE("A mesh's bounds keep up with its vertices")
{
	MeshGeometry subMesh;
	CHECK(subMesh.GetBounds() == BoundingBox(Vector3D(), Vector3D()));

	MeshVertex vertex;
	vertex.Position = Vector3D(2.0f, 3.0f, 4.0f);
	subMesh.AddVertex(vertex);
	// Just that vertex: the box a mesh has before it has any vertices doesn't count
	CHECK(subMesh.GetBounds() == BoundingBox(Vector3D(2.0f, 3.0f, 4.0f), Vector3D(2.0f, 3.0f, 4.0f)));

	vertex.Position = Vector3D(-1.0f, 5.0f, 4.0f);
	subMesh.AddVertex(vertex);
	CHECK(subMesh.GetBounds() == BoundingBox(Vector3D(-1.0f, 3.0f, 4.0f), Vector3D(2.0f, 5.0f, 4.0f)));

	subMesh.SetVertexData(std::vector<MeshVertex>{ vertex });
	CHECK(subMesh.GetBounds() == BoundingBox(Vector3D(-1.0f, 5.0f, 4.0f), Vector3D(-1.0f, 5.0f, 4.0f)));
}

TEST_CASE("A render object's bounds are its mesh's box, placed by its transform")
{
	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(MakeMesh({ k_unitBox }));
	CheckBox(object->GetLocalBounds(), Vector3D(-1.0f), Vector3D(1.0f));
	CheckBox(object->GetBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	object->SetTransform(MakeTransform(Vector3D(5.0f, 0.0f, 0.0f)));
	CheckBox(object->GetBounds(), Vector3D(4.0f, -1.0f, -1.0f), Vector3D(6.0f, 1.0f, 1.0f));
	CheckBox(object->GetDraws()[0].Bounds, Vector3D(4.0f, -1.0f, -1.0f), Vector3D(6.0f, 1.0f, 1.0f));
	// Moving doesn't change its own extents
	CheckBox(object->GetLocalBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	// Turned and scaled: the same as the box around the transformed corners
	const Matrix4x4 transform = MakeTransform(Vector3D(0.0f, 2.0f, 0.0f), Vector3D(20.0f, 35.0f, 0.0f), Vector3D(1.0f, 3.0f, 1.0f));
	object->SetTransform(transform);
	const BoundingBox aroundCorners = BoundingBox::Around(k_unitBox.GetTransformedCorners(transform));
	CheckBox(object->GetBounds(), aroundCorners.GetMin(), aroundCorners.GetMax());
	CheckBox(object->GetDraws()[0].Bounds, aroundCorners.GetMin(), aroundCorners.GetMax());

	scene.DestroyRenderObject(object);
}

TEST_CASE("Each draw's bounds follow the object, inside its bounds")
{
	RenderScene scene;
	const BoundingBox left(Vector3D(-3.0f, 0.0f, 0.0f), Vector3D(-2.0f, 1.0f, 1.0f));
	const BoundingBox right(Vector3D(2.0f, 0.0f, 0.0f), Vector3D(3.0f, 1.0f, 1.0f));
	RenderObjectPtr object = scene.CreateRenderObject(MakeMesh({ left, right }));

	object->SetTransform(MakeTransform(Vector3D(0.0f, 10.0f, 0.0f)));
	const std::vector<MeshDraw>& draws = object->GetDraws();
	REQUIRE(draws.size() == 2);
	CheckBox(draws[0].Bounds, Vector3D(-3.0f, 10.0f, 0.0f), Vector3D(-2.0f, 11.0f, 1.0f));
	CheckBox(draws[1].Bounds, Vector3D(2.0f, 10.0f, 0.0f), Vector3D(3.0f, 11.0f, 1.0f));
	CheckBox(object->GetBounds(), Vector3D(-3.0f, 10.0f, 0.0f), Vector3D(3.0f, 11.0f, 1.0f));

	// Asked again after a move, the corners have moved too
	object->SetTransform(MakeTransform(Vector3D(0.0f, -10.0f, 0.0f)));
	CHECK(object->GetDraws()[1].Corners[0].Y() == doctest::Approx(-10.0f));
	CheckBox(object->GetDraws()[1].Bounds, Vector3D(2.0f, -10.0f, 0.0f), Vector3D(3.0f, -9.0f, 1.0f));

	scene.DestroyRenderObject(object);
}

TEST_CASE("A sub-mesh placed twice is drawn and bounded at each placement")
{
	std::vector<MeshPlacement> placements(2);
	placements[0].LocalTransform = MakeTransform(Vector3D(-5.0f, 0.0f, 0.0f));
	placements[1].LocalTransform = MakeTransform(Vector3D(5.0f, 0.0f, 0.0f));

	std::vector<StaticMesh::SubMesh> subMeshes;
	subMeshes.push_back(MakeSubMesh(k_unitBox));
	const std::shared_ptr<const StaticMesh> mesh = StaticMesh::Create("Test", std::move(subMeshes), MakeSlots(1), std::move(placements));
	CHECK(mesh->GetSubMeshes().size() == 1);
	CheckBox(mesh->GetBounds(), Vector3D(-6.0f, -1.0f, -1.0f), Vector3D(6.0f, 1.0f, 1.0f));

	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(StaticMeshInstance::Create(mesh));
	object->SetTransform(MakeTransform(Vector3D(0.0f, 10.0f, 0.0f)));

	const std::vector<MeshDraw>& draws = object->GetDraws();
	REQUIRE(draws.size() == 2);
	CHECK(draws[0].Geometry == draws[1].Geometry);
	CheckBox(draws[0].Bounds, Vector3D(-6.0f, 9.0f, -1.0f), Vector3D(-4.0f, 11.0f, 1.0f));
	CheckBox(draws[1].Bounds, Vector3D(4.0f, 9.0f, -1.0f), Vector3D(6.0f, 11.0f, 1.0f));
	CheckBox(object->GetBounds(), Vector3D(-6.0f, 9.0f, -1.0f), Vector3D(6.0f, 11.0f, 1.0f));

	// Each placement draws with the object's transform times its own, with the inverse kept beside it
	const Matrix4x4& second = *draws[1].Transform;
	CHECK(second.GetPosition().X() == doctest::Approx(5.0f));
	CHECK(second.GetPosition().Y() == doctest::Approx(10.0f));
	const Matrix4x4& secondInverse = *(&second + 1);
	CHECK((second * secondInverse).GetPosition().X() == doctest::Approx(0.0f));
	CHECK(draws[0].Transform != draws[1].Transform);

	scene.DestroyRenderObject(object);
}

TEST_CASE("Placements at the mesh's origin draw with the object's own transform")
{
	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(MakeMesh({ k_unitBox, k_unitBox }));
	object->SetTransform(MakeTransform(Vector3D(3.0f, 0.0f, 0.0f)));

	// The same address, so a stage uploads it once for both
	const std::vector<MeshDraw>& draws = object->GetDraws();
	REQUIRE(draws.size() == 2);
	CHECK(draws[0].Transform == &object->GetTransform());
	CHECK(draws[1].Transform == &object->GetTransform());

	scene.DestroyRenderObject(object);
}

TEST_CASE("A render object with no sub-meshes takes up no space")
{
	RenderScene scene;
	RenderObjectPtr nothing = scene.CreateRenderObject(MakeMesh({}));
	nothing->SetTransform(MakeTransform(Vector3D(50.0f, 0.0f, 0.0f)));
	CHECK(nothing->GetLocalBounds().IsEmpty());
	CHECK(nothing->GetBounds().IsEmpty());
	CHECK(scene.GetBounds().IsEmpty());

	// And doesn't pull the scene's bounds out to where it is, or to the origin
	RenderObjectPtr box = scene.CreateRenderObject(MakeMesh({ k_unitBox }));
	box->SetTransform(MakeTransform(Vector3D(10.0f, 0.0f, 0.0f)));
	CheckBox(scene.GetBounds(), Vector3D(9.0f, -1.0f, -1.0f), Vector3D(11.0f, 1.0f, 1.0f));

	scene.ClearObjects();
}

TEST_CASE("A scene's bounds follow its objects being added, moved, given new meshes and removed")
{
	RenderScene scene;
	CHECK(scene.GetBounds().IsEmpty());

	RenderObjectPtr first = scene.CreateRenderObject(MakeMesh({ k_unitBox }));
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	RenderObjectPtr second = scene.CreateRenderObject(MakeMesh({ k_unitBox }));
	second->SetTransform(MakeTransform(Vector3D(10.0f, 0.0f, 0.0f)));
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(11.0f, 1.0f, 1.0f));

	// Moving inward shrinks it, not only moving outward growing it
	U64 revision = scene.GetRevision();
	second->SetTransform(MakeTransform(Vector3D(3.0f, 0.0f, 0.0f)));
	CHECK(scene.GetRevision() > revision);
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(4.0f, 1.0f, 1.0f));

	// Setting the transform it already has changes nothing
	revision = scene.GetRevision();
	second->SetTransform(MakeTransform(Vector3D(3.0f, 0.0f, 0.0f)));
	CHECK(scene.GetRevision() == revision);
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(4.0f, 1.0f, 1.0f));

	second->SetMeshInstance(MakeMesh({ BoundingBox(Vector3D(-5.0f), Vector3D(5.0f)) }));
	CheckBox(scene.GetBounds(), Vector3D(-2.0f, -5.0f, -5.0f), Vector3D(8.0f, 5.0f, 5.0f));

	scene.DestroyRenderObject(second);
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	scene.ClearObjects();
	CHECK(scene.GetBounds().IsEmpty());
}

TEST_CASE("Objects drawing the same mesh share it rather than copying it")
{
	std::vector<StaticMesh::SubMesh> subMeshes;
	subMeshes.push_back(MakeSubMesh(k_unitBox));
	const std::shared_ptr<const StaticMesh> mesh = StaticMesh::Create("Shared", std::move(subMeshes), MakeSlots(1));

	RenderScene scene;
	RenderObjectPtr first = scene.CreateRenderObject(StaticMeshInstance::Create(mesh));
	RenderObjectPtr second = scene.CreateRenderObject(StaticMeshInstance::Create(mesh));
	second->SetTransform(MakeTransform(Vector3D(10.0f, 0.0f, 0.0f)));

	// Both draw the mesh's own geometry, each where its object is
	REQUIRE(first->GetDraws().size() == 1);
	REQUIRE(second->GetDraws().size() == 1);
	CHECK(first->GetDraws()[0].Geometry == &mesh->GetSubMeshes()[0].Geometry);
	CHECK(second->GetDraws()[0].Geometry == &mesh->GetSubMeshes()[0].Geometry);
	// But each draws with its own object's slots
	CHECK(first->GetDraws()[0].Slot == &first->GetMeshInstance()->GetMaterialSlots()[0]);
	CHECK(second->GetDraws()[0].Slot == &second->GetMeshInstance()->GetMaterialSlots()[0]);
	CheckBox(second->GetDraws()[0].Bounds, Vector3D(9.0f, -1.0f, -1.0f), Vector3D(11.0f, 1.0f, 1.0f));

	scene.ClearObjects();
}

TEST_CASE("Swapping an object's mesh keeps the object and lets the old mesh go")
{
	RenderScene scene;
	std::shared_ptr<StaticMeshInstance> small = MakeMesh({ k_unitBox });
	const std::weak_ptr<StaticMeshInstance> oldInstance = small;
	RenderObjectPtr object = scene.CreateRenderObject(std::move(small));
	object->SetTransform(MakeTransform(Vector3D(0.0f, 5.0f, 0.0f)));
	const U32 liveID = object->GetLiveID();
	REQUIRE(object->GetDraws().size() == 1);

	const BoundingBox left(Vector3D(-3.0f, 0.0f, 0.0f), Vector3D(-2.0f, 1.0f, 1.0f));
	const BoundingBox right(Vector3D(2.0f, 0.0f, 0.0f), Vector3D(3.0f, 1.0f, 1.0f));
	object->SetMeshInstance(MakeMesh({ left, right }));

	// The same object, drawing the new mesh where it already was
	CHECK(object->GetLiveID() == liveID);
	REQUIRE(object->GetDraws().size() == 2);
	CheckBox(object->GetDraws()[1].Bounds, Vector3D(2.0f, 5.0f, 0.0f), Vector3D(3.0f, 6.0f, 1.0f));
	CheckBox(object->GetBounds(), Vector3D(-3.0f, 5.0f, 0.0f), Vector3D(3.0f, 6.0f, 1.0f));
	// Nothing else held the old instance
	CHECK(oldInstance.expired());

	scene.ClearObjects();
}

TEST_CASE("Sub-meshes sharing a material share a slot, which their draws point at")
{
	// Three sub-meshes, the first and last sharing slot 0
	std::vector<StaticMesh::SubMesh> subMeshes;
	subMeshes.push_back(MakeSubMesh(k_unitBox, 0));
	subMeshes.push_back(MakeSubMesh(k_unitBox, 1));
	subMeshes.push_back(MakeSubMesh(k_unitBox, 0));
	const std::shared_ptr<const StaticMesh> mesh = StaticMesh::Create("Slots", std::move(subMeshes), MakeSlots(2));

	const std::shared_ptr<StaticMeshInstance> instance = StaticMeshInstance::Create(mesh);
	const std::vector<MaterialSlot>& slots = instance->GetMaterialSlots();
	REQUIRE(slots.size() == 2);
	CHECK(slots[0].GetName() == "Slot0");
	CHECK(slots[1].GetName() == "Slot1");

	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(instance);
	const std::vector<MeshDraw>& draws = object->GetDraws();
	REQUIRE(draws.size() == 3);
	CHECK(draws[0].Slot == &slots[0]);
	CHECK(draws[1].Slot == &slots[1]);
	CHECK(draws[2].Slot == &slots[0]);

	scene.ClearObjects();
}

TEST_CASE("An object without a mesh draws nothing and takes up no space")
{
	RenderScene scene;
	RenderObjectPtr object = scene.CreateRenderObject(nullptr);
	object->SetTransform(MakeTransform(Vector3D(50.0f, 0.0f, 0.0f)));
	CHECK(object->GetDraws().empty());
	CHECK(object->GetBounds().IsEmpty());
	CHECK(scene.GetBounds().IsEmpty());

	// Given one, it draws it; and taking it away again leaves it drawing nothing
	object->SetMeshInstance(MakeMesh({ k_unitBox }));
	CHECK(object->GetDraws().size() == 1);
	object->SetMeshInstance(nullptr);
	CHECK(object->GetDraws().empty());
	CHECK(object->GetBounds().IsEmpty());

	scene.ClearObjects();
}
