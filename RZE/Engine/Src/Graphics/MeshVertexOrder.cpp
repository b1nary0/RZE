#include <Graphics/MeshVertexOrder.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/DebugUtils/Debug.h>

#include <limits>
#include <utility>

void MeshVertexOrder::OrderByFirstUse(std::vector<MeshVertex>& vertices, std::vector<U32>& indices)
{
	constexpr U32 k_notPlaced = std::numeric_limits<U32>::max();

	// Where each vertex goes
	std::vector<U32> newIndices(vertices.size(), k_notPlaced);
	U32 nextIndex = 0;
	for (U32& index : indices)
	{
		AssertExpr(index < vertices.size());
		U32& newIndex = newIndices[index];
		if (newIndex == k_notPlaced)
		{
			newIndex = nextIndex++;
		}
		index = newIndex;
	}

	for (U32& newIndex : newIndices)
	{
		if (newIndex == k_notPlaced)
		{
			newIndex = nextIndex++;
		}
	}

	std::vector<MeshVertex> ordered(vertices.size());
	for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		ordered[newIndices[vertexIndex]] = vertices[vertexIndex];
	}
	vertices = std::move(ordered);
}
