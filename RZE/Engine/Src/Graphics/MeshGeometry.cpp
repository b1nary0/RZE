#include <StdAfx.h>
#include <Graphics/MeshGeometry.h>

#include <Graphics/Material.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <GLM/common.hpp>

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
	AllocateData(BuildInterleavedVertexData(m_vertices));
}

void MeshGeometry::AllocateData(std::vector<float>&& interleavedVertexData)
{
	AssertExpr(m_vertexBuffer == nullptr);
	AssertExpr(m_indexBuffer == nullptr);

	CalculateBounds();

	m_vertexBuffer = std::make_shared<VertexBuffer>();
	m_vertexBuffer->Initialize(std::move(interleavedVertexData), sizeof(TempDataLayoutStructure));

	m_indexBuffer = std::make_shared<IndexBuffer>();
	m_indexBuffer->Initialize(m_indices);
}

std::vector<float> MeshGeometry::BuildInterleavedVertexData(const std::vector<MeshVertex>& vertices)
{
	std::vector<float> vertexDataBuffer;
	vertexDataBuffer.reserve(vertices.size() * (sizeof(TempDataLayoutStructure) / sizeof(float)));
	for (const MeshVertex& vertex : vertices)
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

	return vertexDataBuffer;
}

void MeshGeometry::CalculateBounds()
{
	if (m_vertices.empty())
	{
		m_boundsMin = Vector3D();
		m_boundsMax = Vector3D();
		return;
	}

	glm::vec3 boundsMin = m_vertices[0].Position.GetInternalVec();
	glm::vec3 boundsMax = boundsMin;
	for (const MeshVertex& vertex : m_vertices)
	{
		boundsMin = glm::min(boundsMin, vertex.Position.GetInternalVec());
		boundsMax = glm::max(boundsMax, vertex.Position.GetInternalVec());
	}

	m_boundsMin = Vector3D(boundsMin.x, boundsMin.y, boundsMin.z);
	m_boundsMax = Vector3D(boundsMax.x, boundsMax.y, boundsMax.z);
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

void MeshGeometry::SetVertexData(std::vector<MeshVertex>&& verts)
{
	m_vertices = std::move(verts);
}

void MeshGeometry::SetIndexData(const std::vector<U32>& indices)
{
	m_indices = indices;
}

void MeshGeometry::SetIndexData(std::vector<U32>&& indices)
{
	m_indices = std::move(indices);
}

void MeshGeometry::SetMaterial(const std::shared_ptr<MaterialInstance>& material)
{
	AssertNotNull(material);
	m_material = material;
}

std::shared_ptr<const MaterialInstance> MeshGeometry::GetMaterial() const
{
	AssertNotNull(m_material);
	return m_material;
}

const MaterialInstance& MeshGeometry::GetMaterialRef() const
{
	AssertNotNull(m_material);
	return *m_material;
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

const std::shared_ptr<VertexBuffer>& MeshGeometry::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

const std::shared_ptr<IndexBuffer>& MeshGeometry::GetIndexBuffer() const
{
	return m_indexBuffer;
}
