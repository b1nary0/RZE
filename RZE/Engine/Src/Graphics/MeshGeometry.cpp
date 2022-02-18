#include <StdAfx.h>
#include <Graphics/MeshGeometry.h>

#include <Graphics/Material.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

namespace
{
	struct TempDataLayoutStructure
	{
		Vector3D position;
		Vector3D normal;
		Vector2D uv;
		Vector3D tangent;
	};
}

MeshGeometry::MeshGeometry(U32 vertexCount, U32 indexCount)
{
	m_vertices.reserve(vertexCount);
	m_indices.reserve(indexCount);
}

MeshGeometry::~MeshGeometry()
{
}

void MeshGeometry::AllocateData()
{
	AssertExpr(m_vertexBuffer == nullptr);
	AssertExpr(m_indexBuffer == nullptr);

	std::vector<float> vertexDataBuffer;
	vertexDataBuffer.reserve(m_vertices.size() * sizeof(MeshVertex));
	for (const MeshVertex& vertex : GetVertices())
	{
		// #TODO
		// These copies are in a different order than how we write/read with the import pipeline
		for (int index = 0; index < 3; ++index)
		{
			vertexDataBuffer.push_back(vertex.Position[index]);
		}

		for (int index = 0; index < 3; ++index)
		{
			vertexDataBuffer.push_back(vertex.Normal[index]);
		}

		for (int index = 0; index < 2; ++index)
		{
			vertexDataBuffer.push_back(vertex.UVData[index]);
		}

		for (int index = 0; index < 3; ++index)
		{
			vertexDataBuffer.push_back(vertex.Tangent[index]);
		}
	}

	m_vertexBuffer = std::make_shared<VertexBuffer>();
	
	m_vertexBuffer->Initialize(std::move(vertexDataBuffer), sizeof(TempDataLayoutStructure));

	m_indexBuffer = std::make_shared<IndexBuffer>();
	m_indexBuffer->Initialize(m_indices);
}

void MeshGeometry::AddVertex(const MeshVertex& vertex)
{
	m_vertices.push_back(vertex);
}

void MeshGeometry::AddIndex(U32 index)
{
	m_indices.push_back(index);
}

void MeshGeometry::SetName(const std::string& name)
{
	m_name = name;
}

void MeshGeometry::SetVertexData(const std::vector<MeshVertex>& verts)
{
	m_vertices = verts;
}

void MeshGeometry::SetIndexData(const std::vector<U32>& indices)
{
	m_indices = indices;
}

void MeshGeometry::SetMaterial(const std::shared_ptr<MaterialInstance>& material)
{
	AssertNotNull(material);
	m_material = material;
}

const std::shared_ptr<MaterialInstance> MeshGeometry::GetMaterial() const
{
	AssertNotNull(m_material);
	return m_material;
}

std::shared_ptr<MaterialInstance> MeshGeometry::GetMaterial()
{
	AssertNotNull(m_material);
	return m_material;
}

const std::vector<MeshVertex>& MeshGeometry::GetVertices()
{
	return m_vertices;
}

const std::vector<float>& MeshGeometry::GetVertexDataRaw() const
{
	return m_vertexBuffer->GetData();
}

const std::vector<U32>& MeshGeometry::GetIndexDataRaw() const
{
	return m_indexBuffer->GetData();
}

const std::shared_ptr<VertexBuffer> MeshGeometry::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

const std::shared_ptr<IndexBuffer> MeshGeometry::GetIndexBuffer() const
{
	return m_indexBuffer;
}
