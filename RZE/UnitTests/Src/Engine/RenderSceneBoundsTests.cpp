#include <doctest/doctest.h>

#include <Graphics/RenderScene.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>

#include <vector>

// Meshes are built on the CPU here: their vertices are enough for their bounds, and nothing in these tests draws

namespace
{
	// A sub-mesh with a vertex at each corner of box
	MeshGeometry MakeSubMesh(const BoundingBox& box)
	{
		MeshGeometry subMesh;
		for (const Vector3D& corner : box.GetCorners())
		{
			MeshVertex vertex;
			vertex.Position = corner;
			subMesh.AddVertex(vertex);
		}
		return subMesh;
	}

	StaticMeshInstance MakeMesh(const std::vector<BoundingBox>& subMeshBoxes)
	{
		std::vector<MeshGeometry> subMeshes;
		for (const BoundingBox& box : subMeshBoxes)
		{
			subMeshes.push_back(MakeSubMesh(box));
		}

		StaticMeshInstance mesh;
		mesh.Initialize(subMeshes);
		return mesh;
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
	CheckBox(object->GetSubMeshBounds()[0].Bounds, Vector3D(4.0f, -1.0f, -1.0f), Vector3D(6.0f, 1.0f, 1.0f));
	// Moving doesn't change its own extents
	CheckBox(object->GetLocalBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	// Turned and scaled: the same as the box around the transformed corners
	const Matrix4x4 transform = MakeTransform(Vector3D(0.0f, 2.0f, 0.0f), Vector3D(20.0f, 35.0f, 0.0f), Vector3D(1.0f, 3.0f, 1.0f));
	object->SetTransform(transform);
	const BoundingBox aroundCorners = BoundingBox::Around(k_unitBox.GetTransformedCorners(transform));
	CheckBox(object->GetBounds(), aroundCorners.GetMin(), aroundCorners.GetMax());
	CheckBox(object->GetSubMeshBounds()[0].Bounds, aroundCorners.GetMin(), aroundCorners.GetMax());

	scene.DestroyRenderObject(object);
}

TEST_CASE("Each sub-mesh's bounds follow the object, inside its bounds")
{
	RenderScene scene;
	const BoundingBox left(Vector3D(-3.0f, 0.0f, 0.0f), Vector3D(-2.0f, 1.0f, 1.0f));
	const BoundingBox right(Vector3D(2.0f, 0.0f, 0.0f), Vector3D(3.0f, 1.0f, 1.0f));
	RenderObjectPtr object = scene.CreateRenderObject(MakeMesh({ left, right }));

	object->SetTransform(MakeTransform(Vector3D(0.0f, 10.0f, 0.0f)));
	const std::vector<RenderObject::SubMeshBounds>& subMeshBounds = object->GetSubMeshBounds();
	REQUIRE(subMeshBounds.size() == 2);
	CheckBox(subMeshBounds[0].Bounds, Vector3D(-3.0f, 10.0f, 0.0f), Vector3D(-2.0f, 11.0f, 1.0f));
	CheckBox(subMeshBounds[1].Bounds, Vector3D(2.0f, 10.0f, 0.0f), Vector3D(3.0f, 11.0f, 1.0f));
	CheckBox(object->GetBounds(), Vector3D(-3.0f, 10.0f, 0.0f), Vector3D(3.0f, 11.0f, 1.0f));

	// Asked again after a move, the corners have moved too
	object->SetTransform(MakeTransform(Vector3D(0.0f, -10.0f, 0.0f)));
	CHECK(object->GetSubMeshBounds()[1].Corners[0].Y() == doctest::Approx(-10.0f));
	CheckBox(object->GetSubMeshBounds()[1].Bounds, Vector3D(2.0f, -10.0f, 0.0f), Vector3D(3.0f, -9.0f, 1.0f));

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

	second->SetStaticMesh(MakeMesh({ BoundingBox(Vector3D(-5.0f), Vector3D(5.0f)) }));
	CheckBox(scene.GetBounds(), Vector3D(-2.0f, -5.0f, -5.0f), Vector3D(8.0f, 5.0f, 5.0f));

	scene.DestroyRenderObject(second);
	CheckBox(scene.GetBounds(), Vector3D(-1.0f), Vector3D(1.0f));

	scene.ClearObjects();
	CHECK(scene.GetBounds().IsEmpty());
}
