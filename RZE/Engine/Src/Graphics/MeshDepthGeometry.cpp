#include <Graphics/MeshDepthGeometry.h>

#include <Graphics/MeshGeometry.h>
#include <Graphics/MeshVertexOrder.h>

#include <Utils/DebugUtils/Debug.h>

#include <array>
#include <cstring>
#include <limits>
#include <unordered_map>

namespace
{
	// A position's floats' bits
	using PositionBits = std::array<U32, 3>;

	struct PositionBitsHash
	{
		size_t operator()(const PositionBits& bits) const
		{
			// FNV-1a over the three words
			U64 hash = 14695981039346656037ull;
			for (const U32 word : bits)
			{
				hash = (hash ^ word) * 1099511628211ull;
			}
			return static_cast<size_t>(hash);
		}
	};

	PositionBits GetBits(const Vector3D& position)
	{
		const float values[3] = { position.X(), position.Y(), position.Z() };
		PositionBits bits;
		std::memcpy(bits.data(), values, sizeof(values));
		return bits;
	}
}

void MeshDepthGeometry::WeldPositions(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices, std::vector<Vector3D>& outPositions, std::vector<U32>& outIndices)
{
	constexpr U32 k_notWelded = std::numeric_limits<U32>::max();

	outPositions.clear();
	outIndices.clear();
	outIndices.reserve(indices.size());

	// Each vertex's welded index, once an index has used it
	std::vector<U32> weldedOf(vertices.size(), k_notWelded);
	std::unordered_map<PositionBits, U32, PositionBitsHash> weldedAt;
	weldedAt.reserve(vertices.size());

	for (const U32 index : indices)
	{
		AssertExpr(index < vertices.size());
		U32& welded = weldedOf[index];
		if (welded == k_notWelded)
		{
			const Vector3D& position = vertices[index].Position;
			const PositionBits bits = GetBits(position);
			const auto found = weldedAt.find(bits);
			if (found != weldedAt.end())
			{
				welded = found->second;
			}
			else
			{
				welded = static_cast<U32>(outPositions.size());
				weldedAt.emplace(bits, welded);
				outPositions.push_back(position);
			}
		}
		outIndices.push_back(welded);
	}
}

void MeshDepthGeometry::Build(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices, std::vector<Vector3D>& outPositions, std::vector<U32>& outIndices)
{
	WeldPositions(vertices, indices, outPositions, outIndices);
	MeshVertexOrder::ImproveCacheLocality(outIndices, static_cast<U32>(outPositions.size()), k_cacheSize);
	MeshVertexOrder::OrderByFirstUse(outPositions, outIndices);
}
