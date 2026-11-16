#include <doctest/doctest.h>

#include <Graphics/MeshGeometry.h>
#include <Graphics/MeshVertexOrder.h>

#include <algorithm>
#include <array>
#include <deque>
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

namespace
{
	using Triangle = std::array<U32, 3>;

	// A triangle with its smallest index first, so rotations of one compare equal and a flipped one doesn't
	Triangle Canonical(Triangle triangle)
	{
		std::rotate(triangle.begin(), std::min_element(triangle.begin(), triangle.end()), triangle.end());
		return triangle;
	}

	std::vector<Triangle> CanonicalTriangles(const std::vector<U32>& indices)
	{
		std::vector<Triangle> triangles;
		for (size_t corner = 0; corner + 2 < indices.size(); corner += 3)
		{
			triangles.push_back(Canonical({ indices[corner], indices[corner + 1], indices[corner + 2] }));
		}
		std::sort(triangles.begin(), triangles.end());
		return triangles;
	}

	// A cellsPerSide x cellsPerSide grid of quads, two triangles each, with the triangles in a fixed scrambled order
	// so the input is a bad one for the cache
	std::vector<U32> MakeScrambledGrid(U32 cellsPerSide, U32& outVertexCount)
	{
		const U32 verticesPerSide = cellsPerSide + 1;
		outVertexCount = verticesPerSide * verticesPerSide;

		std::vector<Triangle> triangles;
		for (U32 row = 0; row < cellsPerSide; ++row)
		{
			for (U32 column = 0; column < cellsPerSide; ++column)
			{
				const U32 topLeft = row * verticesPerSide + column;
				const U32 bottomLeft = topLeft + verticesPerSide;
				triangles.push_back({ topLeft, bottomLeft, topLeft + 1 });
				triangles.push_back({ topLeft + 1, bottomLeft, bottomLeft + 1 });
			}
		}

		// A fixed-seed shuffle, so the test always sees the same input
		U32 state = 12345;
		for (size_t index = triangles.size() - 1; index > 0; --index)
		{
			state = state * 1664525u + 1013904223u;
			std::swap(triangles[index], triangles[(state >> 8) % (index + 1)]);
		}

		std::vector<U32> indices;
		for (const Triangle& triangle : triangles)
		{
			indices.insert(indices.end(), triangle.begin(), triangle.end());
		}
		return indices;
	}

	// How many vertex fetches miss a first-in first-out cache of cacheSize entries
	U32 CountCacheMisses(const std::vector<U32>& indices, U32 cacheSize)
	{
		std::deque<U32> cache;
		U32 misses = 0;
		for (const U32 index : indices)
		{
			if (std::find(cache.begin(), cache.end(), index) == cache.end())
			{
				++misses;
				cache.push_back(index);
				if (cache.size() > cacheSize)
				{
					cache.pop_front();
				}
			}
		}
		return misses;
	}
}

TEST_CASE("Positions are stored in the order the indices first use them, as vertices are")
{
	std::vector<Vector3D> positions;
	for (U32 index = 0; index < 5; ++index)
	{
		positions.push_back(Vector3D(static_cast<float>(index), 0.0f, 0.0f));
	}
	// Vertex 1 isn't used
	std::vector<U32> indices = { 3, 0, 4, 4, 0, 2 };

	MeshVertexOrder::OrderByFirstUse(positions, indices);

	CHECK(indices == std::vector<U32>{ 0, 1, 2, 2, 1, 3 });
	const float expectedOrder[] = { 3.0f, 0.0f, 4.0f, 2.0f, 1.0f };
	REQUIRE(positions.size() == 5);
	for (size_t index = 0; index < positions.size(); ++index)
	{
		CHECK(positions[index].X() == expectedOrder[index]);
	}
}

TEST_CASE("Improving cache locality keeps every triangle with its winding, and only changes their order")
{
	U32 vertexCount = 0;
	std::vector<U32> indices = MakeScrambledGrid(8, vertexCount);
	const std::vector<U32> before = indices;

	MeshVertexOrder::ImproveCacheLocality(indices, vertexCount, 12);

	REQUIRE(indices.size() == before.size());
	// The same triangles, each as a rotation of one it was (a flipped one would have another canonical form)
	CHECK(CanonicalTriangles(indices) == CanonicalTriangles(before));

	// Each is put out as it was, so even the rotation is kept
	std::vector<Triangle> asBefore;
	std::vector<Triangle> asAfter;
	for (size_t corner = 0; corner < before.size(); corner += 3)
	{
		asBefore.push_back({ before[corner], before[corner + 1], before[corner + 2] });
		asAfter.push_back({ indices[corner], indices[corner + 1], indices[corner + 2] });
	}
	std::sort(asBefore.begin(), asBefore.end());
	std::sort(asAfter.begin(), asAfter.end());
	CHECK(asBefore == asAfter);
}

TEST_CASE("Improving cache locality gives the same order every time")
{
	U32 vertexCount = 0;
	const std::vector<U32> original = MakeScrambledGrid(8, vertexCount);

	std::vector<U32> first = original;
	std::vector<U32> second = original;
	MeshVertexOrder::ImproveCacheLocality(first, vertexCount, 12);
	MeshVertexOrder::ImproveCacheLocality(second, vertexCount, 12);

	CHECK(first == second);
}

TEST_CASE("A scrambled grid has far fewer cache misses after improving its cache locality")
{
	U32 vertexCount = 0;
	std::vector<U32> indices = MakeScrambledGrid(16, vertexCount);
	const U32 missesBefore = CountCacheMisses(indices, 12);

	MeshVertexOrder::ImproveCacheLocality(indices, vertexCount, 12);
	const U32 missesAfter = CountCacheMisses(indices, 12);

	CHECK(missesAfter <= missesBefore);
	// A 16 x 16 grid has 289 vertices, and an ideal order misses a little over that
	CHECK(missesAfter < missesBefore / 2);
	CHECK(missesAfter < 2 * vertexCount);
}

TEST_CASE("Triangles that already use the cache well don't get worse")
{
	// A strip, in order
	std::vector<U32> indices;
	for (U32 quad = 0; quad < 20; ++quad)
	{
		indices.insert(indices.end(), { 2 * quad, 2 * quad + 1, 2 * quad + 2, 2 * quad + 2, 2 * quad + 1, 2 * quad + 3 });
	}
	const std::vector<U32> before = indices;
	const U32 missesBefore = CountCacheMisses(before, 12);

	MeshVertexOrder::ImproveCacheLocality(indices, 42, 12);

	CHECK(CanonicalTriangles(indices) == CanonicalTriangles(before));
	CHECK(CountCacheMisses(indices, 12) <= missesBefore);
}

TEST_CASE("Improving cache locality copes with no triangles, degenerate ones and stray indices")
{
	std::vector<U32> none;
	MeshVertexOrder::ImproveCacheLocality(none, 0, 12);
	CHECK(none.empty());

	// A triangle with a vertex at two corners, two triangles apart, and two indices past the last whole triangle
	std::vector<U32> indices = { 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 2 };
	const std::vector<U32> before = indices;
	MeshVertexOrder::ImproveCacheLocality(indices, 5, 12);

	REQUIRE(indices.size() == before.size());
	CHECK(CanonicalTriangles(std::vector<U32>(indices.begin(), indices.begin() + 9)) == CanonicalTriangles(std::vector<U32>(before.begin(), before.begin() + 9)));
	CHECK(indices[9] == 4);
	CHECK(indices[10] == 2);
}
