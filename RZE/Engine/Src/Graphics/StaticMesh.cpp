#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

StaticMesh::StaticMesh(const std::vector<MeshGeometry>& subMeshes)
{
	mSubMeshes = std::move(subMeshes);
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::SetMeshes(const std::vector<MeshGeometry>& meshGeometry)
{
	mSubMeshes = std::move(meshGeometry);
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes()
{
	return mSubMeshes;
}

