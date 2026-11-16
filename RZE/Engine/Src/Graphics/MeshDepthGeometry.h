#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

class Vector3D;
struct MeshVertex;

// The geometry a depth-only pass draws a mesh from (DepthOnlyMeshBatches): its positions alone, welded, in vertex-cache
// order. A depth test's result doesn't depend on the order triangles are drawn in or on what a vertex carries besides
// its position, so this has fewer and smaller vertices than the geometry the forward pass draws.
namespace MeshDepthGeometry
{
	// How many vertices the order is made for (see MeshVertexOrder::ImproveCacheLocality). The GPUs this runs on keep
	// 16 to 32 or more; a cache of 12 serves any of them well.
	constexpr U32 k_cacheSize = 12;

	// The positions the indices use, with vertices at exactly the same position as one, and the indices renumbered to
	// them. Positions are the same when their floats' bits are, so a position is never changed, and +0 and -0 stay
	// apart: the vertex shader must place a vertex to the bit as the forward pass's does. Vertices no index uses are
	// left out, and the rest are in the order the indices first use them. Every triangle is kept, with its winding.
	void WeldPositions(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices, std::vector<Vector3D>& outPositions, std::vector<U32>& outIndices);

	// WeldPositions, then the triangles in vertex-cache order, then the positions in the order they're first used. Pure
	// CPU work; safe on any thread.
	void Build(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices, std::vector<Vector3D>& outPositions, std::vector<U32>& outIndices);
}
