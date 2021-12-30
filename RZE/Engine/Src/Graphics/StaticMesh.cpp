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
	mSubMeshes = std::move(meshGeometry);
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes() const
{
	return mSubMeshes;
}

std::vector<MeshGeometry>& StaticMesh::GetSubMeshes()
{
	return mSubMeshes;
}
