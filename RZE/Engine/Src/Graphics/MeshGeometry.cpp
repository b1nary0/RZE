#include <StdAfx.h>
#include <Graphics/MeshGeometry.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <Utils/Math/Math.h>

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

void MeshGeometry::AllocateData(const std::vector<float>& interleavedVertexData)
{
	AssertExpr(m_vertexBuffer == nullptr);
	AssertExpr(m_indexBuffer == nullptr);

	// Again, in case the vertices were filled in some other way
	CalculateBounds();

	m_vertexBuffer = std::make_shared<VertexBuffer>();
	m_vertexBuffer->Initialize(interleavedVertexData, sizeof(TempDataLayoutStructure));

	m_indexBuffer = std::make_shared<IndexBuffer>();
	m_indexBuffer->Initialize(m_indices);

	// The uploads took their own copies. Swapped with empty vectors, since clear() keeps the memory.
	std::vector<MeshVertex>().swap(m_vertices);
	std::vector<U32>().swap(m_indices);
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
		m_bounds = BoundingBox(Vector3D(), Vector3D());
		return;
	}

	m_bounds = BoundingBox();
	for (const MeshVertex& vertex : m_vertices)
	{
		m_bounds.Grow(vertex.Position);
	}
}

void MeshGeometry::AddVertex(const MeshVertex& vertex)
{
	AssertMsg(!IsAllocated(), "Vertices can't be added once the geometry is on the GPU");
	m_vertices.push_back(vertex);
	if (m_vertices.size() == 1)
	{
		m_bounds = BoundingBox(vertex.Position, vertex.Position);
	}
	else
	{
		m_bounds.Grow(vertex.Position);
	}
}

void MeshGeometry::AddIndex(U32 index)
{
	AssertMsg(!IsAllocated(), "Indices can't be added once the geometry is on the GPU");
	m_indices.push_back(index);
}

void MeshGeometry::SetName(const std::string& name)
{
	m_name = name;
}

void MeshGeometry::SetVertexData(const std::vector<MeshVertex>& verts)
{
	AssertMsg(!IsAllocated(), "Vertices can't be set once the geometry is on the GPU");
	m_vertices = verts;
	CalculateBounds();
}

void MeshGeometry::SetVertexData(std::vector<MeshVertex>&& verts)
{
	AssertMsg(!IsAllocated(), "Vertices can't be set once the geometry is on the GPU");
	m_vertices = std::move(verts);
	CalculateBounds();
}

void MeshGeometry::SetIndexData(const std::vector<U32>& indices)
{
	AssertMsg(!IsAllocated(), "Indices can't be set once the geometry is on the GPU");
	m_indices = indices;
}

void MeshGeometry::SetIndexData(std::vector<U32>&& indices)
{
	AssertMsg(!IsAllocated(), "Indices can't be set once the geometry is on the GPU");
	m_indices = std::move(indices);
}

const std::shared_ptr<VertexBuffer>& MeshGeometry::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

const std::shared_ptr<IndexBuffer>& MeshGeometry::GetIndexBuffer() const
{
	return m_indexBuffer;
}
