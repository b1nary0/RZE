#pragma once

#include <memory>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class IndexBuffer;
class VertexBuffer;

struct MeshVertex
{
	Vector3D Position;
	Vector3D Normal;
	Vector3D Tangent;
	Vector2D UVData;
};

// One draw of a mesh's sub-mesh geometry. The same geometry can be placed more than once (an instanced part
// in an FBX), each time with its own transform relative to the mesh.
struct MeshPlacement
{
	U32 GeometryIndex = 0;
	Matrix4x4 LocalTransform;
};

class MeshGeometry
{
public:
	MeshGeometry() = default;
	MeshGeometry(U32 vertexCount, U32 indexCount);
	~MeshGeometry();

	// What depth-only passes draw it from (DepthOnlyMeshBatches): the positions alone, welded, with their own indices
	// (MeshDepthGeometry)
	struct DepthData
	{
		// X, Y, Z of each vertex
		std::vector<float> Positions;
		std::vector<U32> Indices;
	};

	// Creates the GPU vertex/index buffers, for drawing and for depth, then frees the CPU copies of the vertices and
	// indices: nothing reads them once they're uploaded. The bounds stay. Main thread only.
	void AllocateData();
	// As above, using vertex data already laid out by BuildInterleavedVertexData and depth data from BuildDepthData
	// (e.g. prepared on a worker thread).
	void AllocateData(const std::vector<float>& interleavedVertexData, const DepthData& depthData);

	// Converts vertices into the layout the vertex shader expects. Pure CPU work; safe on any thread.
	static std::vector<float> BuildInterleavedVertexData(const std::vector<MeshVertex>& vertices);
	// What a geometry's vertices and indices are drawn from for depth alone. Pure CPU work; safe on any thread.
	static DepthData BuildDepthData(const std::vector<MeshVertex>& vertices, const std::vector<U32>& indices);

	// Building the geometry, before AllocateData; asserts after it
	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetName(const std::string& name);
	void SetVertexData(const std::vector<MeshVertex>& verts);
	void SetVertexData(std::vector<MeshVertex>&& verts);
	void SetIndexData(const std::vector<U32>& indices);
	void SetIndexData(std::vector<U32>&& indices);

	const std::string& GetName() const { return m_name; }

	// Object-space box around the vertices, kept up to date as they're added or set; a zero-sized box at the origin
	// with none
	const BoundingBox& GetBounds() const { return m_bounds; }

	const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const;
	const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const;

	// Binds its vertices and indices and draws them instanced: instanceCount instances from firstInstance in the
	// instance stream bound beside them. After AllocateData.
	void Draw(U32 instanceCount, U32 firstInstance) const;
	// The same, from the depth buffers: positions only, so for a pass whose vertex shader reads nothing else
	// (MeshInstanceFormats::DepthPosition). After AllocateData.
	void DrawDepth(U32 instanceCount, U32 firstInstance) const;

private:
	void CalculateBounds();
	// Its vertices and indices are on the GPU, and the CPU copies are gone
	bool IsAllocated() const { return m_vertexBuffer != nullptr; }

private:
	std::string m_name;
	// Only until AllocateData uploads them
	std::vector<MeshVertex> m_vertices;
	BoundingBox m_bounds { Vector3D(), Vector3D() };
	// Only until AllocateData uploads them
	std::vector<U32> m_indices;

	std::shared_ptr<VertexBuffer> m_vertexBuffer;
	std::shared_ptr<IndexBuffer> m_indexBuffer;
	// The same triangles as DepthData has them
	std::shared_ptr<VertexBuffer> m_depthVertexBuffer;
	std::shared_ptr<IndexBuffer> m_depthIndexBuffer;
};