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

	void AllocateData();

	void AddVertex(const MeshVertex& vertex);
	void AddIndex(U32 index);

	void SetName(const std::string& name);
	void SetVertexData(const std::vector<MeshVertex>& verts);
	void SetIndexData(const std::vector<U32>& indices);

	void SetMaterial(const std::shared_ptr<MaterialInstance>& material);

	const std::string& GetName() const { return m_name; }
	std::shared_ptr<MaterialInstance> GetMaterial();
	std::shared_ptr<const MaterialInstance> GetMaterial() const;
	const std::vector<MeshVertex>& GetVertices();

	const std::vector<float>& GetVertexDataRaw() const;
	const std::vector<U32>& GetIndexDataRaw() const;

	const std::shared_ptr<VertexBuffer> GetVertexBuffer() const;
	const std::shared_ptr<IndexBuffer> GetIndexBuffer() const;

private:
	std::string m_name;
	std::vector<MeshVertex> m_vertices;
	std::vector<U32> m_indices;

	std::shared_ptr<VertexBuffer> m_vertexBuffer;
	std::shared_ptr<IndexBuffer> m_indexBuffer;
	std::shared_ptr<MaterialInstance> m_material;
};