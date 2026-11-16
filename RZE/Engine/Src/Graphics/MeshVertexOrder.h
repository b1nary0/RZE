#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

class Vector3D;
struct MeshVertex;

namespace MeshVertexOrder
{
	// Stores a geometry's vertices in the order its indices first use them, and renumbers the indices to match, so a
	// draw fetches vertices in about the order they lie in memory. What's drawn doesn't change: each triangle keeps
	// its vertices and winding, and the triangles keep their order. Vertices no index uses keep their order, after
	// the rest.
	void OrderByFirstUse(std::vector<MeshVertex>& vertices, std::vector<U32>& indices);
	// The same, for positions alone
	void OrderByFirstUse(std::vector<Vector3D>& positions, std::vector<U32>& indices);

	// Puts the triangles in an order that reuses vertices a vertex cache of cacheSize entries still holds (Tipsify:
	// Sander, Nehab and Barczak, "Fast Triangle Reordering for Vertex Locality and Reduced Overdraw", 2007; the
	// algorithm of Assimp's aiProcess_ImproveCacheLocality). The vertices don't change. Each triangle keeps its
	// corners in their order, so its winding stays. The same input gives the same order. Any indices past the last
	// whole triangle keep their place, at the end.
	void ImproveCacheLocality(std::vector<U32>& indices, U32 vertexCount, U32 cacheSize);
}
