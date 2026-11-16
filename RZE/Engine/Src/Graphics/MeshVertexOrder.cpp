#include <Graphics/MeshVertexOrder.h>

#include <Graphics/MeshGeometry.h>

#include <Utils/DebugUtils/Debug.h>

#include <cstddef>
#include <limits>
#include <utility>

namespace
{
	constexpr U32 k_none = std::numeric_limits<U32>::max();

	template <typename Vertex>
	void OrderVerticesByFirstUse(std::vector<Vertex>& vertices, std::vector<U32>& indices)
	{
		// Where each vertex goes
		std::vector<U32> newIndices(vertices.size(), k_none);
		U32 nextIndex = 0;
		for (U32& index : indices)
		{
			AssertExpr(index < vertices.size());
			U32& newIndex = newIndices[index];
			if (newIndex == k_none)
			{
				newIndex = nextIndex++;
			}
			index = newIndex;
		}

		for (U32& newIndex : newIndices)
		{
			if (newIndex == k_none)
			{
				newIndex = nextIndex++;
			}
		}

		std::vector<Vertex> ordered(vertices.size());
		for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
		{
			ordered[newIndices[vertexIndex]] = vertices[vertexIndex];
		}
		vertices = std::move(ordered);
	}
}

void MeshVertexOrder::OrderByFirstUse(std::vector<MeshVertex>& vertices, std::vector<U32>& indices)
{
	OrderVerticesByFirstUse(vertices, indices);
}

void MeshVertexOrder::OrderByFirstUse(std::vector<Vector3D>& positions, std::vector<U32>& indices)
{
	OrderVerticesByFirstUse(positions, indices);
}

void MeshVertexOrder::ImproveCacheLocality(std::vector<U32>& indices, U32 vertexCount, U32 cacheSize)
{
	const size_t triangleCount = indices.size() / 3;
	const size_t cornerCount = triangleCount * 3;
	if (triangleCount == 0)
	{
		return;
	}

	// The triangles each vertex is a corner of, a run in vertexTriangles starting at runStart[vertex]. A triangle with
	// one vertex at two corners is in its run twice.
	std::vector<U32> runStart(static_cast<size_t>(vertexCount) + 1, 0);
	for (size_t corner = 0; corner < cornerCount; ++corner)
	{
		AssertExpr(indices[corner] < vertexCount);
		++runStart[indices[corner] + 1];
	}
	for (size_t vertex = 0; vertex < vertexCount; ++vertex)
	{
		runStart[vertex + 1] += runStart[vertex];
	}

	std::vector<U32> vertexTriangles(cornerCount);
	std::vector<U32> runFill(runStart.begin(), runStart.end() - 1);
	for (size_t corner = 0; corner < cornerCount; ++corner)
	{
		vertexTriangles[runFill[indices[corner]]++] = static_cast<U32>(corner / 3);
	}

	// Corners of triangles not yet put out, for each vertex
	std::vector<U32> liveCorners(vertexCount);
	for (size_t vertex = 0; vertex < vertexCount; ++vertex)
	{
		liveCorners[vertex] = runStart[vertex + 1] - runStart[vertex];
	}

	// When each vertex last entered the modelled cache, on a clock that ticks once per vertex that enters. A vertex
	// is in the cache if it entered within the last cacheSize ticks.
	std::vector<U32> enteredAt(vertexCount, 0);
	U32 clock = cacheSize + 1;

	std::vector<bool> isPutOut(triangleCount, false);
	std::vector<U32> deadEnds;
	std::vector<U32> neighbours;
	std::vector<U32> ordered;
	ordered.reserve(indices.size());

	U32 nextScanned = 0;
	U32 fanning = 0;
	while (fanning != k_none)
	{
		// Every triangle still left around the fanning vertex, in the order they were made
		neighbours.clear();
		for (U32 run = runStart[fanning]; run < runStart[fanning + 1]; ++run)
		{
			const U32 triangle = vertexTriangles[run];
			if (isPutOut[triangle])
			{
				continue;
			}
			isPutOut[triangle] = true;

			for (U32 corner = 0; corner < 3; ++corner)
			{
				const U32 vertex = indices[triangle * 3 + corner];
				ordered.push_back(vertex);
				deadEnds.push_back(vertex);
				neighbours.push_back(vertex);
				--liveCorners[vertex];
				if (clock - enteredAt[vertex] > cacheSize)
				{
					enteredAt[vertex] = clock++;
				}
			}
		}

		// Next fan from the neighbour with triangles left that is most likely still in the cache after those, or else
		// the most recently seen vertex that has any, or else the first vertex with any
		U32 best = k_none;
		U32 bestPriority = 0;
		for (const U32 vertex : neighbours)
		{
			if (liveCorners[vertex] == 0)
			{
				continue;
			}

			// 0 for one that its own triangles would push out of the cache, else how recently it entered
			U32 priority = 0;
			if (clock - enteredAt[vertex] + 2 * liveCorners[vertex] <= cacheSize)
			{
				priority = clock - enteredAt[vertex];
			}
			if (best == k_none || priority > bestPriority)
			{
				bestPriority = priority;
				best = vertex;
			}
		}

		while (best == k_none && !deadEnds.empty())
		{
			const U32 vertex = deadEnds.back();
			deadEnds.pop_back();
			if (liveCorners[vertex] > 0)
			{
				best = vertex;
			}
		}

		while (best == k_none && nextScanned < vertexCount)
		{
			if (liveCorners[nextScanned] > 0)
			{
				best = nextScanned;
			}
			++nextScanned;
		}

		fanning = best;
	}

	// Whole triangles were put out as they were, and any indices past them stay at the end
	ordered.insert(ordered.end(), indices.begin() + static_cast<std::ptrdiff_t>(cornerCount), indices.end());
	indices = std::move(ordered);
}
