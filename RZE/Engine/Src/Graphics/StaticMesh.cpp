#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::Initialize(const std::vector<MeshGeometry>&& meshGeometry)
{
	m_subMeshes = std::move(meshGeometry);

	// @TODO form the vertex buffer here?
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes() const
{
	return m_subMeshes;
}

std::vector<MeshGeometry>& StaticMesh::GetSubMeshes()
{
	return m_subMeshes;
}
