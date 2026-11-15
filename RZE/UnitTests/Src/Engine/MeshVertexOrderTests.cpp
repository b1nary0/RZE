#include <doctest/doctest.h>

#include <Graphics/MeshGeometry.h>
#include <Graphics/MeshVertexOrder.h>

#include <vector>

namespace
{
	// Each vertex is told apart by its position's X
	std::vector<MeshVertex> MakeVertices(U32 count)
	{
		std::vector<MeshVertex> vertices(count);
		for (U32 vertexIndex = 0; vertexIndex < count; ++vertexIndex)
		{
			vertices[vertexIndex].Position = Vector3D(static_cast<float>(vertexIndex), 0.0f, 0.0f);
		}
		return vertices;
	}

	// Which original vertex each index names, corner by corner
	std::vector<float> CornerVertices(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices)
	{
		std::vector<float> corners;
		for (U32 index : indices)
		{
			corners.push_back(vertices[index].Position.X());
		}
		return corners;
	}
}

TEST_CASE("Vertices are stored in the order the indices first use them, and every triangle keeps its corners")
{
	std::vector<MeshVertex> vertices = MakeVertices(7);
	// Vertices 2 and 6 aren't used
	std::vector<U32> indices = { 3, 1, 4, 1, 5, 4, 0, 3, 5 };
	const std::vector<float> cornersBefore = CornerVertices(vertices, indices);

	MeshVertexOrder::OrderByFirstUse(vertices, indices);

	CHECK(indices == std::vector<U32>{ 0, 1, 2, 1, 3, 2, 4, 0, 3 });
	CHECK(CornerVertices(vertices, indices) == cornersBefore);

	// The unused ones come last, in their old order
	REQUIRE(vertices.size() == 7);
	const float expectedOrder[] = { 3.0f, 1.0f, 4.0f, 5.0f, 0.0f, 2.0f, 6.0f };
	for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		CHECK(vertices[vertexIndex].Position.X() == expectedOrder[vertexIndex]);
	}
}

TEST_CASE("Vertices already in first-use order stay as they are")
{
	std::vector<MeshVertex> vertices = MakeVertices(4);
	std::vector<U32> indices = { 0, 1, 2, 2, 1, 3 };

	MeshVertexOrder::OrderByFirstUse(vertices, indices);

	CHECK(indices == std::vector<U32>{ 0, 1, 2, 2, 1, 3 });
	for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		CHECK(vertices[vertexIndex].Position.X() == static_cast<float>(vertexIndex));
	}
}

TEST_CASE("Ordering a geometry with no indices keeps its vertices")
{
	std::vector<MeshVertex> vertices = MakeVertices(3);
	std::vector<U32> indices;

	MeshVertexOrder::OrderByFirstUse(vertices, indices);

	CHECK(indices.empty());
	REQUIRE(vertices.size() == 3);
	CHECK(vertices[0].Position.X() == 0.0f);
	CHECK(vertices[2].Position.X() == 2.0f);
}
