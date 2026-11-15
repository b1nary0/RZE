#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

struct MeshVertex;

namespace MeshVertexOrder
{
	// Stores a geometry's vertices in the order its indices first use them, and renumbers the indices to match, so a
	// draw fetches vertices in about the order they lie in memory. What's drawn doesn't change: each triangle keeps
	// its vertices and winding, and the triangles keep their order. Vertices no index uses keep their order, after
	// the rest.
	void OrderByFirstUse(std::vector<MeshVertex>& vertices, std::vector<U32>& indices);
}
