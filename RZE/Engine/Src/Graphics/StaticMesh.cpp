#include <StdAfx.h>
#include <Graphics/StaticMesh.h>

StaticMesh::StaticMesh(const std::vector<MeshGeometry>& subMeshes)
{
	mSubMeshes = std::move(subMeshes);
}

StaticMesh::~StaticMesh()
{
}

const std::vector<MeshGeometry>& StaticMesh::GetSubMeshes()
{
	return mSubMeshes;
}
