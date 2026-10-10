#pragma once

#include <memory>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

class MaterialInstance;
class IndexBuffer;
class VertexBuffer;

struct MeshVertex
{
	Vector3D Position;
	Vector3D Normal;
	Vector3D Tangent;
	Vector2D UVData;
};

class MeshGeometry
{
public:
	MeshGeometry() = default;
	MeshGeometry(U32 vertexCount, U32 indexCount);
	~MeshGeometry();

	// Creates the GPU vertex/index buffers. Main thread only.
	void AllocateData();
	// As above, using vertex data already laid out by BuildInterleavedVertexData (e.g. prepared on a worker thread).
	void AllocateData(std::vector<float>&& interleavedVertexData);

	// Converts vertices into the layout the vertex shader expects. Pure CPU work; safe on any thread.
	static std::vector<float> BuildInterleavedVertexData(const std::vector<MeshVertex>& vertices);

	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetName(const std::string& name);
	void SetVertexData(const std::vector<MeshVertex>& verts);
	void SetVertexData(std::vector<MeshVertex>&& verts);
	void SetIndexData(const std::vector<U32>& indices);
	void SetIndexData(std::vector<U32>&& indices);

	void SetMaterial(const std::shared_ptr<MaterialInstance>& material);

	const std::string& GetName() const { return m_name; }
	std::shared_ptr<MaterialInstance> GetMaterial();
	std::shared_ptr<const MaterialInstance> GetMaterial() const;
	// For drawing: reads the material without copying the shared_ptr
	const MaterialInstance& GetMaterialRef() const;
	const std::vector<MeshVertex>& GetVertices();

	// Object-space axis-aligned bounds of the vertices; calculated in AllocateData()
	const Vector3D& GetBoundsMin() const { return m_boundsMin; }
	const Vector3D& GetBoundsMax() const { return m_boundsMax; }

	const std::vector<float>& GetVertexDataRaw() const;
	const std::vector<U32>& GetIndexDataRaw() const;

	const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const;
	const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const;

private:
	void CalculateBounds();

private:
	std::string m_name;
	std::vector<MeshVertex> m_vertices;
	Vector3D m_boundsMin;
	Vector3D m_boundsMax;
	std::vector<U32> m_indices;

	std::shared_ptr<VertexBuffer> m_vertexBuffer;
	std::shared_ptr<IndexBuffer> m_indexBuffer;
	std::shared_ptr<MaterialInstance> m_material;
};