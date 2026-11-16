#include <doctest/doctest.h>

#include <Graphics/MeshDepthGeometry.h>
#include <Graphics/MeshGeometry.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
	MeshVertex MakeVertex(const Vector3D& position, float normalX = 0.0f, float u = 0.0f)
	{
		MeshVertex vertex;
		vertex.Position = position;
		vertex.Normal = Vector3D(normalX, 1.0f, 0.0f);
		vertex.UVData = Vector2D(u, 0.0f);
		return vertex;
	}

	// The positions at a triangle's corners, as the geometry has them
	std::vector<Vector3D> CornerPositions(const std::vector<Vector3D>& positions, const std::vector<U32>& indices)
	{
		std::vector<Vector3D> corners;
		for (const U32 index : indices)
		{
			corners.push_back(positions[index]);
		}
		return corners;
	}

	bool SameBits(float a, float b)
	{
		return std::signbit(a) == std::signbit(b) && a == b;
	}
}

TEST_CASE("Vertices at exactly the same position are welded, whatever else they carry")
{
	// A quad as an exporter writes it: its two triangles' shared corners are separate vertices, with their own UVs and
	// normals
	const std::vector<MeshVertex> vertices =
	{
		MakeVertex(Vector3D(0.0f, 0.0f, 0.0f), 0.0f, 0.0f),
		MakeVertex(Vector3D(1.0f, 0.0f, 0.0f), 0.0f, 1.0f),
		MakeVertex(Vector3D(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		MakeVertex(Vector3D(1.0f, 0.0f, 0.0f), 1.0f, 0.5f),
		MakeVertex(Vector3D(0.0f, 1.0f, 0.0f), 1.0f, 0.5f),
		MakeVertex(Vector3D(1.0f, 1.0f, 0.0f), 1.0f, 1.0f),
	};
	const std::vector<U32> indices = { 0, 1, 2, 3, 5, 4 };

	std::vector<Vector3D> positions;
	std::vector<U32> welded;
	MeshDepthGeometry::WeldPositions(vertices, indices, positions, welded);

	REQUIRE(positions.size() == 4);
	CHECK(welded == std::vector<U32>{ 0, 1, 2, 1, 3, 2 });
	CHECK(CornerPositions(positions, welded)[3] == Vector3D(1.0f, 0.0f, 0.0f));
}

TEST_CASE("Positions that differ by one bit, or by the sign of a zero, stay apart")
{
	const float x = 1.0f;
	const float nextX = std::nextafter(x, 2.0f);
	const std::vector<MeshVertex> vertices =
	{
		MakeVertex(Vector3D(x, 0.0f, 0.0f)),
		MakeVertex(Vector3D(nextX, 0.0f, 0.0f)),
		MakeVertex(Vector3D(x, 0.0f, 0.0f)),
		MakeVertex(Vector3D(x, -0.0f, 0.0f)),
		MakeVertex(Vector3D(x, 0.0f, 0.0f)),
		MakeVertex(Vector3D(x, 0.0f, -0.0f)),
	};
	const std::vector<U32> indices = { 0, 1, 2, 3, 4, 5 };

	std::vector<Vector3D> positions;
	std::vector<U32> welded;
	MeshDepthGeometry::WeldPositions(vertices, indices, positions, welded);

	// Vertices 0, 2 and 4 are one; the others are each their own
	REQUIRE(positions.size() == 4);
	CHECK(welded == std::vector<U32>{ 0, 1, 0, 2, 0, 3 });
	CHECK(positions[1].X() == nextX);
	CHECK(SameBits(positions[2].Y(), -0.0f));
	CHECK(SameBits(positions[3].Z(), -0.0f));
	// Not moved to the other zero or the other float
	CHECK(SameBits(positions[0].Y(), 0.0f));
}

TEST_CASE("Welding keeps every triangle, degenerate ones included, and leaves out vertices no index uses")
{
	const std::vector<MeshVertex> vertices =
	{
		MakeVertex(Vector3D(5.0f, 5.0f, 5.0f)),
		MakeVertex(Vector3D(0.0f, 0.0f, 0.0f)),
		MakeVertex(Vector3D(1.0f, 0.0f, 0.0f)),
		MakeVertex(Vector3D(0.0f, 0.0f, 0.0f)),
	};
	// Vertex 0 is unused. The second triangle's corners are all one position once welded.
	const std::vector<U32> indices = { 1, 2, 3, 1, 3, 1 };

	std::vector<Vector3D> positions;
	std::vector<U32> welded;
	MeshDepthGeometry::WeldPositions(vertices, indices, positions, welded);

	CHECK(positions.size() == 2);
	REQUIRE(welded.size() == indices.size());
	CHECK(welded == std::vector<U32>{ 0, 1, 0, 0, 0, 0 });
}

TEST_CASE("Welding no indices gives nothing")
{
	const std::vector<MeshVertex> vertices = { MakeVertex(Vector3D(1.0f, 2.0f, 3.0f)) };

	std::vector<Vector3D> positions = { Vector3D(1.0f) };
	std::vector<U32> welded = { 7 };
	MeshDepthGeometry::WeldPositions(vertices, {}, positions, welded);

	CHECK(positions.empty());
	CHECK(welded.empty());
}

TEST_CASE("A built depth geometry draws the same triangles, with welded positions in the order its triangles first use them")
{
	// A 6 x 6 grid of quads, each its own four vertices (every corner shared by neighbours is in the list once per
	// quad), with the triangles in a column-major order
	constexpr U32 k_cells = 6;
	std::vector<MeshVertex> vertices;
	std::vector<U32> indices;
	for (U32 column = 0; column < k_cells; ++column)
	{
		for (U32 row = 0; row < k_cells; ++row)
		{
			const U32 first = static_cast<U32>(vertices.size());
			const float x = static_cast<float>(column);
			const float y = static_cast<float>(row);
			vertices.push_back(MakeVertex(Vector3D(x, y, 0.0f), 0.0f, 0.0f));
			vertices.push_back(MakeVertex(Vector3D(x + 1.0f, y, 0.0f), 0.0f, 1.0f));
			vertices.push_back(MakeVertex(Vector3D(x, y + 1.0f, 0.0f), 1.0f, 0.0f));
			vertices.push_back(MakeVertex(Vector3D(x + 1.0f, y + 1.0f, 0.0f), 1.0f, 1.0f));
			indices.insert(indices.end(), { first, first + 1, first + 2, first + 2, first + 1, first + 3 });
		}
	}

	std::vector<Vector3D> positions;
	std::vector<U32> built;
	MeshDepthGeometry::Build(vertices, indices, positions, built);

	// 7 x 7 distinct positions from 144 vertices
	CHECK(positions.size() == (k_cells + 1) * (k_cells + 1));
	REQUIRE(built.size() == indices.size());

	// The same triangles by position, each with the same winding: a triangle's corners cyclically as they were
	auto trianglesOf = [](const std::vector<Vector3D>& triangleCorners, std::vector<std::array<float, 6>>& out)
	{
		for (size_t corner = 0; corner + 2 < triangleCorners.size(); corner += 3)
		{
			size_t smallest = 0;
			for (size_t offset = 1; offset < 3; ++offset)
			{
				const Vector3D& a = triangleCorners[corner + offset];
				const Vector3D& b = triangleCorners[corner + smallest];
				if (a.X() < b.X() || (a.X() == b.X() && a.Y() < b.Y()))
				{
					smallest = offset;
				}
			}
			std::array<float, 6> key;
			for (size_t offset = 0; offset < 3; ++offset)
			{
				const Vector3D& position = triangleCorners[corner + (smallest + offset) % 3];
				key[offset * 2] = position.X();
				key[offset * 2 + 1] = position.Y();
			}
			out.push_back(key);
		}
		std::sort(out.begin(), out.end());
	};
	std::vector<Vector3D> originalCorners;
	for (const U32 index : indices)
	{
		originalCorners.push_back(vertices[index].Position);
	}
	std::vector<std::array<float, 6>> originalTriangles;
	std::vector<std::array<float, 6>> builtTriangles;
	trianglesOf(originalCorners, originalTriangles);
	trianglesOf(CornerPositions(positions, built), builtTriangles);
	CHECK(builtTriangles == originalTriangles);

	// First-use order: each index is either one already used or the next new one
	U32 nextNew = 0;
	for (const U32 index : built)
	{
		REQUIRE(index <= nextNew);
		if (index == nextNew)
		{
			++nextNew;
		}
	}
	CHECK(nextNew == positions.size());
}

TEST_CASE("A geometry's depth data is flat positions, with indices into them")
{
	const std::vector<MeshVertex> vertices =
	{
		MakeVertex(Vector3D(1.0f, 2.0f, 3.0f)),
		MakeVertex(Vector3D(4.0f, 5.0f, 6.0f)),
		MakeVertex(Vector3D(1.0f, 2.0f, 3.0f), 1.0f),
		MakeVertex(Vector3D(7.0f, 8.0f, 9.0f)),
	};
	const std::vector<U32> indices = { 0, 1, 3, 1, 2, 3 };

	const MeshGeometry::DepthData data = MeshGeometry::BuildDepthData(vertices, indices);

	CHECK(data.Positions.size() == 3 * 3);
	CHECK(data.Indices.size() == 6);
	for (const U32 index : data.Indices)
	{
		CHECK(index < 3);
	}
	// Each triangle's corners are where they were
	for (size_t corner = 0; corner < indices.size(); ++corner)
	{
		CAPTURE(corner);
		const Vector3D& expected = vertices[indices[corner]].Position;
		const size_t index = data.Indices[corner];
		CHECK(data.Positions[index * 3] == expected.X());
		CHECK(data.Positions[index * 3 + 1] == expected.Y());
		CHECK(data.Positions[index * 3 + 2] == expected.Z());
	}
}
