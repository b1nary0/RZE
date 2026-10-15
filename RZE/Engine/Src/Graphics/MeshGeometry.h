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

	// Creates the GPU vertex/index buffers, then frees the CPU copies of the vertices and indices: nothing reads them
	// once they're uploaded. The bounds stay. Main thread only.
	void AllocateData();
	// As above, using vertex data already laid out by BuildInterleavedVertexData (e.g. prepared on a worker thread).
	void AllocateData(const std::vector<float>& interleavedVertexData);

	// Converts vertices into the layout the vertex shader expects. Pure CPU work; safe on any thread.
	static std::vector<float> BuildInterleavedVertexData(const std::vector<MeshVertex>& vertices);

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
};